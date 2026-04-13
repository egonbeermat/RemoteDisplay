#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

// Choose the SerialUSB connection
#define REM_SERIALOUT SerialUSB1

// Comment out to remove ethernet code
//#define USE_ETHERNET

// Choose your Ethernet library here, comment out the one not used
#if defined(USE_ETHERNET)
#define QN_ETHERNET
//#define NATIVE_ETHERNET

// QNEthernet
#if defined(QN_ETHERNET)
#include <QNEthernet.h>
using namespace qindesign::network;
#endif // QN_ETHERNET

// Native Ethernet
#if defined(NATIVE_ETHERNET)
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#endif // NATIVE_ETHERNET
#endif // USE_ETHERNET

#define MAX_PACKET_SIZE(a, b) ((a) > (b) ? (a) : (b))

#define MAX_ETH_PACKET_SIZE 1430
#define MAX_USB_PACKET_SIZE 2022 //Tested - 1002 //2022 //4074

class RemoteDisplay
{
    using touch_callback_t = void (*)(uint16_t x, uint16_t y, uint8_t action);
    using refresh_callback_t = void (*)();
    using command_callback_t = void (*)(uint8_t command);
public:

    enum TouchState {PRESSED, RELEASED};
    enum CommandType {CMD_CONNECT, CMD_DISCONNECT, CMD_DISABLE_SCREEN, CMD_ENABLE_SCREEN};
    enum ConnectionType {SEND_NONE, SEND_ETHERNET, SEND_USBSERIAL};

    void init(uint16_t inScreenWidth, uint16_t inScreenHeight, uint16_t inPortStream = 0);
    void registerRefreshCallback(refresh_callback_t inRefreshCallback);
    void registerTouchCallback(touch_callback_t inTouchCallback);
    void registerCommandCallback(command_callback_t inCommandCallback);

    void pollRemoteCommand();
    void sendData(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2, uint8_t *pixelmap);
#if defined(USE_ETHERNET)
    void connectRemoteEthernet(IPAddress ipRemote);
#endif // USE_ETHERNET
    void connectRemoteSerial();
    void disconnectRemote();

    bool sendRemoteScreen = false;
    bool disableLocalScreen = false;
    ConnectionType connectionType = SEND_NONE;

    uint8_t lastRemoteTouchState = RELEASED;
    uint16_t lastRemoteTouchX = 0;
    uint16_t lastRemoteTouchY = 0;

private:
    const char * serialDelimiter = "DZQZ";
    const uint32_t serialTimeoutMicros = 5'000;
    uint8_t serialFailedCount = 0;

    const uint8_t packetHeaderSize = 14;
    uint16_t screenWidth = 0;
    uint16_t screenHeight = 0;

    enum ControlType {INIT_REMOTE, RLE_ESCAPED_8_HEADER, RLE_ESCAPED_8_DATA};

    #pragma pack(push,1)
        typedef struct {
            //Header
            uint16_t controlValue;
            uint16_t x;
            uint16_t y;
            uint16_t width;
            uint16_t height;
            uint32_t progressStart;
            //
            uint16_t dataBuffer[MAX_PACKET_SIZE(MAX_ETH_PACKET_SIZE, MAX_USB_PACKET_SIZE) / 2];
        } rle_packet_t;
    #pragma pack(pop)

#if defined(USE_ETHERNET)
#if defined(QN_ETHERNET)
EthernetUDP udpStream = EthernetUDP(40);
#endif

#if defined(NATIVE_ETHERNET)
EthernetUDP udpStream;
#endif

IPAddress udpAddress;
#endif // USE_ETHERNET

    uint16_t portStream;

    uint8_t infoBuffer[12];

    rle_packet_t rlePacket;
    const uint8_t * transBufferStart = (uint8_t *)rlePacket.dataBuffer;

    touch_callback_t touchCallback;
    refresh_callback_t refreshCallback;
    command_callback_t commandCallback;

    void connectRemote();
    void processIncomingCommand(const char * incomingPacketBuffer);
    void refreshDisplay();
    void sendHeader(uint16_t controlValue, uint32_t extraData);
    void sendPacket(uint8_t * buffer, uint32_t packetSize);
};
#endif //REMOTE_DISPLAY_H