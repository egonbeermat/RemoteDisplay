
# _RemoteDisplay_, a library to send Teensy 4.1 screen buffers over Ethernet to display on another device

_Version: 0.1.0_

The _RemoteDisplay_ library, in conjunction with the supplied [Windows](https://github.com/egonbeermat/RemoteDisplay/tree/main/clientsoftware/Windows) and [MacOS](https://github.com/egonbeermat/RemoteDisplay/tree/main/clientsoftware/MacOS) client software, provides the ability to remotely display and control your Teensy screen from your desktop.

## Introduction

Demo of this in action, streaming from a Teensy 4.1 running an LVGL driven display, over Ethernet to a client running on a Mac Mini:

https://youtu.be/TxMsTKo4VVM

If your Teensy 4.1 uses a screen buffer and transmits screen updates to an attached physical display using a standard pattern of defining an area x, y, w, h, and a pointer to a 16 bit RGB565 color buffer for the pixels, you can use this library to compress and send that buffer to display on your desktop, using the included client software. Additionally, you can link touch controls on the desktop into your code on the Teensy 4.1, allowing full remote operation of your device.

Use this for operating your device easily whilst developing, or to test display code without having a physical screen to display on, or to test different resolutions on the remote display, without having a physical screen that supports that resolution.

Remote client software allows you to zoom and pan on the local view of your screen, and super-impose pixel grids and display crosshairs to pinpoint screen co-ordinates, taking some of the guess work out of screen design efforts.

Tested with [QNEthernet](https://github.com/ssilverman/QNEthernet/) _(recommended)_ and [NativeEthernet](https://github.com/vjmuzik/NativeEthernet/) libraries. _(Note: there is a #define in remoteDisplay.h that needs to be changed to use NativeEthernet)_

## Setup

This assumes you already have Ethernet connectivity and code setup. Refer to the examples and Ethernet library documentation if you haven't already done so. Follow these **6** steps to implement:

**Step 1:** Uncomment / comment out the appropriate lines at the start of RemoteDisplay.h to enable QNEthernet or NativeEthernet


**Step 2:** Include the library and declare an instance of RemoteDisplay:

  ```c++
  #include <RemoteDisplay.h>
RemoteDisplay remoteDisplay;
  ```

**Step 3:** After an Ethernet connection is established, initialize the library, providing the width and height of the desired display, and a valid port number to listen for UDP connections:

  ```c++
remoteDisplay.init(SCREENWIDTH, SCREENHEIGHT, portNumber);
  ```

**Step 4:** Register callbacks to be executed when the client software requests a full display refresh (essential but optional), and detects a touch (optional):

  ```c++
void  refreshDisplayCallback() {
	// Called when the remote client requires a full screen refresh, such as on
	// initial connection. No parameters
}
void  remoteTouchCallback(uint16_t x, uint16_t y, uint8_t action) {
	// Executed when remoteDisplay.readRemoteCommand() detects a touch event.
	// x & y represent the co-ords of the touch, action is either 0 (PRESSED) or 1 (RELEASED)
}
...
remoteDisplay.registerRefreshCallback(refreshDisplayCallback);
remoteDisplay.registerTouchCallback(remoteTouchCallback);
```

**Step 5:** In the main loop of your code, you will need to make a call to allow RemoteDisplay to check if it received a command from the client software (touch event, connect, disconnect, send a full display refresh):

  ```c++
void loop() {
...
	remoteDisplay.pollRemoteCommand();
...
}
```
**Step 6:** To transmit buffer updates to the remote client, if the remote client is connected, call `sendData` with parameters x1, y1, x2, y2 (the bounds of the update rectangle) and a pointer to a buffer **only** containing 16 bit RGB565 color values for the area to be updated:

```c++
if (remoteDisplay.sendRemoteScreen == true) {
    remoteDisplay.sendData(x1, y1, x2, y2, (uint8_t *)buffer);
}
```

## Additional Info

If you registered a touch callback, it will be called if the `pollRemoteCommand()` detected a touch event. Alternatively, you can reference the following in your own (polled) touch interface code, and arbitrate between local and remote touches:

`remoteDisplay.lastRemoteTouchState` - set to `RemoteDisplay::PRESSED` or `RemoteDisplay::RELEASED`
`remoteDisplay.lastRemoteTouchX` - X co-ordinate of last touch sent from remote client
`remoteDisplay.lastRemoteTouchY` - Y co-ordinate of last touch sent from remote client

The client software has an interface that provides a mechanism to control if buffer updates are sent to the physical screen, or not, potentially improving performance for the network send by disabling the local screen buffer flush. This sets `remoteDisplay.disableLocalScreen` to true or false, and you can check this before sending your buffer updates to the physical screen.

The client software typically initiates the connection to the Teensy, but you can call `remoteDisplay.connectRemote(IPAddress)` from the Teensy, supplying it's IP address, to initiate the connection. This is useful if you wish to auto-reconnect after a reboot, for instance.

## Performance

RemoteDisplay uses escaped run-length encoding to compress the data before sending over the network. For performance reasons, it is beneficial to reduce the amount of data transfered over the network, but not at the expense of a complex compression algorithm consuming even more time. RemoteDisplay was tested without compression, with RLE encoding, and with both an 8 bit and 16 bit value for run-length in escaped RLE encoding. In all data scenarios (simple UI, complex UI, images, 24fps full screen video), 8 bit escaped RLE encoding performed the best, achieving the best compression / least overhead for compression, and a significant performance improvement compared to just streaming the raw data. For simple to medium complexity UIs, compression ratios of 70-85% are typical. For MJPEG videos encoded in ffmpeg with quality 5, it achieves around 30-40% compression ratios. The remote client provides statistics on data received, compression ratios and additional information about the encoding, so you can make decisions around changing this, if you wish.

## Limitations

- Only works with 16 bit RGB565 data
- Uses UDP for data communication, which is not secure, nor guaranteed network packet delivery
- Cannot solve world hunger

## Using the examples

To be continued....

## Other notes

To be continued....

## Todo

- Release Android and iOS client software
- Add callback for additional processing for connect/disconnect/enable local screen/disable local screen

## References

* [Remote Display for LVGL Development](https://github.com/CubeCoders/LVGLRemoteServer) - the original inspiration for the Teensy-side code. This has been modified with improved compression (RLE escaped), smaller memory utilization, removal of LVGL dependency and additional UDP messages to help the client software functions. Brand new client software and features developed independently of this original codebase.

## License

This library is distributed under the "AGPL-3.0-or-later" license. Please contact the author if you wish to inquire about other license options.
---

Copyright (c) 2024 Ian Wall
