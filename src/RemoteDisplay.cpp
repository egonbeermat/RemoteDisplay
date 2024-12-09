#include <Arduino.h>
#include "RemoteDisplay.h"

FLASHMEM void RemoteDisplay::init(uint16_t inScreenWidth, uint16_t inScreenHeight, uint16_t inPortStream)
{
    screenWidth = inScreenWidth;
    screenHeight = inScreenHeight;
    portStream = inPortStream;
    sendRemoteScreen = false;
    disableLocalScreen = false;

    Serial.printf("In RemoteDisplay::init, port: %d\n", portStream);

    //Start listening
    bool result = udpStream.begin(portStream);
    if (result != 1) {
        Serial.printf("Error with UDP.begin on port %d\n", portStream);
    }
}

FLASHMEM void  RemoteDisplay::registerTouchCallback(touch_callback_t inTouchCallback)
{
    Serial.printf("In RemoteDisplay::registerTouchCallback\n");

    touchCallback = inTouchCallback;
}

FLASHMEM void  RemoteDisplay::registerRefreshCallback(refresh_callback_t inRefreshCallback)
{
    Serial.printf("In RemoteDisplay::registerRefreshCallback\n");

    refreshCallback = inRefreshCallback;
}

FLASHMEM void RemoteDisplay::connectRemote(IPAddress ipRemote)
{
    Serial.printf("In RemoteDisplay::connectRemote, udpStream.begin, ipAddress: %d.%d.%d.%d, port: %d, blockSize: %d\n", ipRemote[0], ipRemote[1], ipRemote[2], ipRemote[3], portStream, MAX_PACKET_SIZE);

    udpAddress = ipRemote;
    sendRemoteScreen = true;

    sendHeader(INIT_REMOTE, 0);
    refreshDisplay();
}

FLASHMEM void RemoteDisplay::disconnectRemote()
{
    Serial.printf("In RemoteDisplay::disconnectRemote\n");

    sendRemoteScreen = false;
}

FLASHMEM void RemoteDisplay::refreshDisplay()
{
    Serial.printf("In RemoteDisplay::refreshDisplay\n");

    if (refreshCallback) {
        refreshCallback();
    }
}

FASTRUN void RemoteDisplay::sendData(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2, uint8_t *pixelmap)
{
    //Serial.printf("In RemoteDisplay::sendData, area: x1: %ld y1: %ld x2:%ld y2:%ld\n", x1, y1, x2, y2);

    if (sendRemoteScreen == false) {
        return;
    }

    rlePacket.controlValue = RLE_ESCAPED_8_DATA;
    rlePacket.x = x1;
    rlePacket.y = y1;
    rlePacket.width = (x2 - x1 + 1);
    rlePacket.progressStart = 0;

    uint32_t fullHeight = (y2 - y1 + 1);
    uint32_t totalPixels = rlePacket.width * fullHeight;

    sendHeader(RLE_ESCAPED_8_HEADER, totalPixels);

    uint16_t runLength = 1; // Current run length
    uint32_t pixelsInPacket = 0;

    uint8_t * dataBufferPtr = (uint8_t *)transBufferStart;

    uint16_t * pixelPtr = (uint16_t *)pixelmap;
    uint16_t * pixelPtrEnd = pixelPtr + totalPixels;

    // Initialize lastColor with the first pixel color
    uint16_t lastColor = *pixelPtr++;

    // Traverse through the pixel data
    while (pixelPtr < pixelPtrEnd)
    {
        uint16_t color = *pixelPtr;

        bool colorChanged = (pixelPtr == (pixelPtrEnd - 1)) || (color != lastColor);
        bool bufferFull = (dataBufferPtr >= transBufferEndPtr - (runLength == 1 ? 2 : 5));

        if (colorChanged || (runLength == 255) || bufferFull)
        {
            // Add the current run to the RLE buffer
            if (runLength == 1) {
                //Just add the color
                *dataBufferPtr++ = lastColor;
                *dataBufferPtr++ = lastColor >> 8;
            } else {
                //Add color twice, and run length
                *dataBufferPtr++ = lastColor;
                *dataBufferPtr++ = lastColor >> 8;
                *dataBufferPtr++ = lastColor;
                *dataBufferPtr++ = lastColor >> 8;
                *dataBufferPtr++ = runLength;
            }

            pixelsInPacket += runLength;

            // Send the RLE buffer if the buffer is full or if we've processed all pixels
            if (bufferFull)
            {
                sendPacket((uint8_t *)&rlePacket, packetHeaderSize + (dataBufferPtr - (uint8_t *)transBufferStart));

                rlePacket.progressStart += pixelsInPacket;
                pixelsInPacket = 0;
                dataBufferPtr = (uint8_t *)transBufferStart;
            }

            // Reset runLength and update lastColor
            runLength = 1;
            lastColor = color;
        }
        else
        {
            // Continue the current run
            runLength++;
        }
        pixelPtr++;
    }
    //Send last
    if (runLength == 1) {
        //Just add the color
        *dataBufferPtr++ = lastColor;
        *dataBufferPtr++ = lastColor >> 8;
    } else {
        //Add color twice, and run length
        *dataBufferPtr++ = lastColor;
        *dataBufferPtr++ = lastColor >> 8;
        *dataBufferPtr++ = lastColor;
        *dataBufferPtr++ = lastColor >> 8;
        *dataBufferPtr++ = runLength;
    }
    pixelsInPacket += runLength;
    sendPacket((uint8_t *)&rlePacket, packetHeaderSize + (dataBufferPtr - transBufferStart));

    rlePacket.progressStart += pixelsInPacket;

    // Check if the total number of pixels sent matches the expected totalPixels
    if (rlePacket.progressStart != totalPixels) {
        Serial.printf("Error: Discrepancy: Expected %ld pixels, but sent %ld pixels\n", totalPixels, rlePacket.progressStart);
    } else {
        //Serial.printf("All pixels sent successfully\n");
    }
}

FASTRUN void RemoteDisplay::pollRemoteCommand()
{
    //Serial.printf("In RemoteDisplay::pollRemoteCommand\n");

    uint8_t remoteStatus = 0;
    char incomingPacketBuffer[5]; // 1 byte status + 2x 16-bit integers for X and Y

    uint32_t packetSize = udpStream.parsePacket();
    if (packetSize == 5) // We expect a 5-byte packet
    {
        udpStream.read(incomingPacketBuffer, 5);

        // Extract touch data from packet
        remoteStatus = incomingPacketBuffer[0];
        int16_t remoteX = (incomingPacketBuffer[1] << 8) | incomingPacketBuffer[2]; // High byte first
        int16_t remoteY = (incomingPacketBuffer[3] << 8) | incomingPacketBuffer[4]; // High byte first

        switch (remoteStatus) {
            case 0: //Pressed
            case 1: //Released
                lastRemoteTouchX = remoteX;
                lastRemoteTouchY = remoteY;
                lastRemoteTouchState = remoteStatus;
                if (touchCallback) {
                    touchCallback(remoteX, remoteY, lastRemoteTouchState);
                }
                break;
            case 2: //Connect
                connectRemote(udpStream.remoteIP());
                break;
            case 3: //Disconnect
                disconnectRemote();
                break;
            case 4: // Refresh the screen
                refreshDisplay();
                break;
            case 5: //Toggle disable local screen
                disableLocalScreen = !disableLocalScreen;
                break;
        }
    }
}

FASTRUN void RemoteDisplay::sendHeader(uint16_t controlValue, uint32_t extraData)
{
    //Serial.printf("In RemoteDisplay::sendHeader, controlValue: 0x%04X, extraData: %ld\n", controlValue, extraData);

    size_t packetSize = 12;
    memcpy(infoBuffer, &controlValue, 2);           // Control value
    memcpy(infoBuffer + 2, &screenWidth, 2);        // Screen width
    memcpy(infoBuffer + 4, &screenHeight, 2);       // Screen height
    memcpy(infoBuffer + 6, &extraData, 4);          // 32-bit extra data

    //Send the packet via udpStream
    sendPacket(infoBuffer, packetSize);
}

FASTRUN void RemoteDisplay::sendPacket(uint8_t * buffer, uint32_t packetSize)
{
    //Send the packet via udpStream
    int result = udpStream.beginPacket(udpAddress, portStream);
    if (result == 1) {
        udpStream.write(buffer, packetSize);
        result = udpStream.endPacket();
        if (result == 0) {
            Serial.printf("Error from endPacket\n");
        }
    } else {
        Serial.printf("Error from beginPacket, udpAddress: %d.%d.%d.%d, portStream: %d\n", udpAddress[0], udpAddress[1], udpAddress[2], udpAddress[3], portStream);
    }
}
