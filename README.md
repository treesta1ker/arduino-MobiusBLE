# MobiusBLE
Enables connectivity with Mobius devices.

This library supports BLE communication with BLE enabled Mobius devices using the [ArduinoBLE](https://www.arduino.cc/en/Reference/ArduinoBLE) library. Thus MobiusBLE can be used on Arduino boards such as the Arduino MKR WiFi 1010, Arduino UNO WiFi Rev.2, Arduino Nano 33 IoT, and Arduino Nano 33 BLE. For the Arduino MKR WiFi 1010, Arduino UNO WiFi Rev.2, and Arduino Nano 33 IoT boards, it requires the NINA module to be running [Arduino NINA-W102 firmware](https://github.com/arduino/nina-fw) v1.2.0 or later to allow ArduinoBLE and MobiusBLE to function properly.

## LED Usage
When the `MobiusDevice::redLed`, `MobiusDevice::blueLed`, and `MobiusDevice::greenLed` LEDs are configured this library will use them to indicate different statuses.
| LEDs                           | Status |
|--------------------------------|--------|
| blinking purple (red + blue)   | scanning for nearby Mobius Devices  |
| blinking blue                  | attempting to connect to a device   |
| solid green                    | discovering service/characteristics |
| blinking yellow (red + green)  | failed to discover characteristics  |
| blinking red                   | failed to connect / device not found|
| blink light blue (blue + green)| sending a BLE request               |


## Examples
#### Discover
This example shows some debugging and discovering methods for Mobius devices. First it will scan for BLE enabled Mobius devices (expecting just one). Once a device is discovered it will attempt connecting to the device. After successfully connecting it will:
1. read the current "scene" ID
2. start the default feed "scene"
3. read the feed "scene" ID (should be 1)
4. start the normal schedule
5. read the current "scene" ID (should be 0)
#### Control
This example shows how a Mobius device may be controlled with an analog signal. First it will scan for BLE enabled Mobius devices (expecting just one). Once the device is discovered it will check the analog PIN (A0) every 2 seconds for the current state. When a new state is detected it will connect to the Mobius device and set the scene corresponding to the state.

## Dependencies
MobiusBLE is heavily dependent upon the [ArduinoBLE](https://www.arduino.cc/en/Reference/ArduinoBLE) library. It was developed & tested using the version [1.2.0](https://github.com/arduino-libraries/ArduinoBLE/releases/tag/1.2.0).

## Troubleshooting
This library is far from perfect and the functionally can be flaky. Some common issues encountered during development include:
* ArduinoBLE commands *freeze*
* ArduinoBLE commands are continuously unsuccessful
* devices cannot be discovered
* services/characteristics cannot be discovered

To help troubleshoot the `MobiusDevice::debug` flag was added. When this flag is set to `true` the library will print more messages to the serial port.
Some other tricks which were often helpful to workaround issues:
* restart/reset the Arduino device by press and releasing the reset button
* connecting and disconnecting from target Mobius device with the nRF Connect app ([GooglePlay](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en_US&gl=US) / [AppStore](https://apps.apple.com/us/app/nrf-connect/id1054362403))

## License
```
MIT License

Copyright (c) 2021 treesta1ker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

