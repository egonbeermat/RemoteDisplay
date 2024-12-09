
# _RemoteDisplay_, A library to send Teensy 4.1 screen buffers over Ethernet to display on another device

_Version: 0.1.0_

The _RemoteDisplay_ library, in conjunction with the supplied client software, provides the ability to remotely display and control your Teensy screen on another device, such as your desktop or phone.

This library is distributed under the "AGPL-3.0-or-later" license. Please contact the author if you wish to inquire about other license options.

## Introduction

Demo of this in action, streaming from a Teensy 4.1 over Ethernet, running an LVGL driven display:

https://youtu.be/TxMsTKo4VVM

If your Teensy 4.1 uses a screen buffer and transmits screen updates to a physical display using a standard pattern of defining an area x, y, w, h, and a pointer to uint16_t colors for the pixels, you can use this library to compress and send that buffer to display on your desktop or mobile device, using the included client software. Additionally, you can link touch controls on the desktop or mobile device into your code on the Teensy 4.1, allowing full remote operation of your device.

Use this for operating your device easily whilst developing, or to test display code without having a physical screen to display on, or to test different resolutions on the remote display, without having a physical screen that supports that resolution.

Remote client software allows you to zoom and pan on the local view of your screen, and super-impose pixel grids and display crosshairs to pinpoint screen co-ordinates, taking some of the guess work out of screen design efforts.

Tested with [QNEthernet](https://github.com/ssilverman/QNEthernet/) _(recommended)_ and [NativeEthernet](https://github.com/vjmuzik/NativeEthernet/) libraries.

## Setup

This assumes you already have Ethernet connectivity and code setup. Refer to the examples and Ethernet library documentation if you haven't already done so. Follow these **5** steps to implement:

**Step 1:** include the library and declare an instance of RemoteDisplay:

  ```c++
  #include <RemoteDisplay.h>
RemoteDisplay remoteDisplay;
  ```

**Step 2:** After an Ethernet connection is established, initialize the library, providing the width and height of the desired display, and a valid port number to listen for UDP connections:

  ```c++
remoteDisplay.init(SCREENWIDTH, SCREENHEIGHT, portNumber);
  ```

**Step 3:** You can register callbacks to be executed when the client software requests a full display refresh, and detects a touch:

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

**Step 4:** In the main loop of your code, you will need to poll to see if the connection to the remote client received a touch event or a request to connect, disconnect or send a full display refresh:

  ```c++
void loop() {
...
	remoteDisplay.pollRemoteCommand();
...
}
```
**Step 5:** To transit buffer updates to the remote client, if the remote client is connected, call `sendData` with parameters x1, y1, x2, y2 (the bounds of the update rectangle) and a pointer to a buffer containing 16 bit RGB565 color values:

```c++
if (remoteDisplay.sendRemoteScreen == true) {
    remoteDisplay.sendData(x1, y1, x2, y2, (uint8_t *)buffer);
}
```

If you registered a touch callback, it will be called if the `pollRemoteCommand()` detected a touch event. Alternatively, you can reference the following in your own (polled) touch interface code, and arbitrate between local and remote touches:

`remoteDisplay.lastRemoteTouchState` - set to `RemoteDisplay::PRESSED` or `RemoteDisplay::RELEASED`
`remoteDisplay.lastRemoteTouchX` - X co-ordinate of last touch sent from remote client
`remoteDisplay.lastRemoteTouchY` - Y co-ordinate of last touch sent from remote client

## Limitations

- Only works with 16 bit RGB565 data
- Uses UDP for data communication, which is not secure, nor guaranteed network packet delivery
- Cannot solve world hunger

## Using the examples

To be continued....

## Other notes

To be continued....

## Todo

To be continued....

## References

* [Remote Display for LVGL Development](https://github.com/CubeCoders/LVGLRemoteServer) - the original inspiration for the Teensy-side code. This has been modified with improved compression (RLE escaped), smaller memory utilization, removal of LVGL dependency and additional UDP messages to help the client software functions. Brand new client software and features developed independently of this original codebase.
---

Copyright (c) 2024 Ian Wall
