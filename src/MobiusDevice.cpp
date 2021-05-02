/*!
 * This file is part of the MobiusBLE library.
 */

#include "MobiusDevice.h"
#include "MobiusCRC.h"


/*!
 * Unsigned integer of the PIN number for the red LED.
 */
uint16_t MobiusDevice::redLed = 0;
/*!
 * Unsigned integer of the PIN number for the blue LED.
 */
uint16_t MobiusDevice::blueLed = 0;
/*!
 * Unsigned integer of the PIN number for the green LED.
 */
uint16_t MobiusDevice::greenLed = 0;
/*!
 * Unsigned integer of the value to turn ON a LED.
 * Defaults to "LOW"
 */
uint16_t MobiusDevice::ledOn = LOW;
/*!
 * Unsigned integer of the value to turn OFF a LED.
 * Defaults to "HIGH"
 */
uint16_t MobiusDevice::ledOff = HIGH;
/*!
 * Boolean determining whether to log debug statements to "Serial".
 */
bool MobiusDevice::debug = false;

/*!
 * @brief Scan for BLEDevices
 *
 * Performs a scan for nearby BLEDevices which have a name of "MOBIUS".
 * Any found devices will have their corresponding addresses added to the
 * give 'addressBuffer'.
 *
 * @return number of found devices (number of addresses added)
 */
uint8_t MobiusDevice::scanForMobiusDevices(String addressBuffer[]) {
    uint8_t count = 0;
    Serial.print("Scanning for BLE devices");
    if (BLE.begin() && BLE.scanForName("MOBIUS", true)) {
        BLEDevice device;
        uint16_t purple[] = { redLed, blueLed };
        for (uint8_t i = 0; !count && i < 3; i++) {
            blinkLEDs(2, purple);
            Serial.print('.');
            device = BLE.available();
            while (device) {
                // add the new address
                addressBuffer[count++] = device.address();
                if (debug) {
                    Serial.print(" Found ");
                    Serial.print(device.address());
                    Serial.print('\t');
                }
                blinkLEDs(1, purple);
                // check for next device?
                device = BLE.available();
            }
        }
        Serial.println();
        
        if (debug) {
            Serial.print("Device count:");
            Serial.println(count);
        }
    } else {
        Serial.println(" - Failed to start scan");
    }
    BLE.stopScan();
    return count;
}


/*!
 * Default constructor.
 */
MobiusDevice::MobiusDevice() { }

/*!
 * Constructs a new MobiusDevice which has the given address.
 */
MobiusDevice::MobiusDevice(String address) {
    _address = address;
}
/*!
 * De-construct the class.
 */
MobiusDevice::~MobiusDevice() {
    disconnect();
}
/*!
 * @brief Connect to the device.
 *
 * Connect to the device corresponding to the current address and
 * verify it has the required BLE characteristics.
 *
 * @return true only if successfully connected
 */
bool MobiusDevice::connect() {
    // rest the message count/ID
    // starting with 2, because why not?
    _messageId = 2;
    return connectTo(_address);
}
/*!
 * @brief Disconnect from the device.
 *
 * Disconnect from the currently connected device.
 *
 * @return true if no longer connected
 */
bool MobiusDevice::disconnect() {
    bool disconnected = true;
    if (_device) {
        disconnected = _device.disconnect();
        // reset current BLE objects
        _device = BLEDevice();
        _requestChar = BLECharacteristic();
        _responseChar = BLECharacteristic();
    }
    BLE.disconnect();
    return disconnected;
}
/*!
 * @brief Get the currently running scene.
 *
 * Query the device to determine the currently running scene.
 *
 * @return an unsigned short
 */
uint16_t MobiusDevice::getCurrentScene() {
    uint16_t scene = -1;
    uint16_t bodySize;
    uint16_t attSize = sizeof Mobius::ATTRIBUTE_CURRENT_SCENE;
    uint8_t attributes[attSize];
    memcpy(attributes, Mobius::ATTRIBUTE_CURRENT_SCENE, attSize);

    uint8_t* body = getData(attributes, attSize, bodySize);
    scene = body[7];
    scene = (scene << 8) + (body[6]);
    delete[] body;
    return scene;
}
/*!
 * @brief Set a new scene.
 *
 * Sends a set scene request with the given 'sceneId' and verify
 * the response indicates a successful set action.
 *
 * @return true if the 'set' was successful
 */
bool MobiusDevice::setScene(uint16_t sceneId) {
    uint16_t attSize = sizeof Mobius::ATTRIBUTE_SCENE;
    uint8_t attributes[attSize];
    memcpy(attributes, Mobius::ATTRIBUTE_SCENE, attSize);
    // update scene ID portion of the attribute
    attributes[5] = lowByte(sceneId); // little endian
    attributes[6] = highByte(sceneId);// little endian

    return setData(attributes, attSize);
}
/*!
 * @brief Set the default feed scene.
 *
 * Sends a set scene request with the default feed scene ID and
 * verify the response indicates a successful set action.
 *
 * @return true if the 'set' was successful
 */
bool MobiusDevice::setFeedScene() {
    return setScene(Mobius::FEED_SCENE_ID);
}
/*!
 * @brief Run the schedule.
 *
 * Sends a request to set the device into the schedule operational
 * state and verify the response indicates a successful action.
 *
 * @return true if the action was successful
 */
bool MobiusDevice::runSchedule() {
    uint16_t attSize = sizeof Mobius::ATTRIBUTE_OPERATION_STATE;
    uint8_t attributes[attSize];
    memcpy(attributes, Mobius::ATTRIBUTE_OPERATION_STATE, attSize);
    // update which state to set
    attributes[attSize-1] = Mobius::OPERATION_STATE_SCHEDULE;
    
    return setData(attributes, attSize);
}




/*!
 * Attempt to connect to a Mobius device with the given address.
 *
 * @return a connected BLEDevice if successful, otherwise a neutral BLEDevice
 */
bool MobiusDevice::connectTo(String address) {
    _device = BLEDevice();
    Serial.print("Scanning for BLE device (");
    Serial.print(address);
    Serial.print(')');
    uint16_t blue[] = { blueLed };
    // start scanning for a Mobius device with the give address
    for (uint8_t i = 0; !BLE.scanForAddress(address) && i < 4; i++) {
        // attempt to start the scan 4 times before moving on
        Serial.print('.');
        blinkLEDs(1, blue);
        if (debug) {
            Serial.print(" (scan failed to start) ");
        }
    }

    // scan up to 26 times (~13 sec)
    for (uint8_t i = 0; !_device && i < 26; i++) {
        _device = BLE.available();
        if (!_device) {
            // didn't find the device
            Serial.print('.');
            blinkLEDs(1, blue);
        }
    }

    // stop scanning for the device
    BLE.stopScan();
    Serial.println();

    if (_device) {
        Serial.println("Found the BLE device");
        if (!connectTo(_device)) {
            Serial.println("Failed to connect with characteristics");
            // return a non-initialized device
            _device = BLEDevice();
        }
    }
    else {
        Serial.println("BLE device not found");
        uint16_t red[] = { redLed };
        blinkLEDs(2, red);
    }

    return _device;
}
/*!
 * @brief Connect to BLEDevice
 *
 * Connect to the given 'peripheral' and check for characteristics.
 * Multiple attempts may be made to both connect and discover characteristics.
 *
 * @return whether the BLEDevice is currently connected
 */
bool MobiusDevice::connectTo(BLEDevice& peripheral) {
    // assuming peripheral is NOT null
    // connect to the device and check for attributes
    bool hasConnected = false;
    bool charsConnected = false;
    // attempt to connect (should only take one)
    for (uint8_t i = 0; !hasConnected && !charsConnected && i < 2; i++) {
        // turn on green indicating discovery is happening
        if (greenLed) digitalWrite(greenLed, ledOn);
        Serial.print("Connecting ...");
        hasConnected = peripheral.connect();
        if (hasConnected) {
            Serial.println(" Successful");
            // connected, but no attributes yet
            // attempt to discover attributes (should only take one)
            Serial.print("Discovering service ..");
            for (uint8_t j = 0; !charsConnected && j < 3; j++) {
                Serial.print('.');
                if (peripheral.discoverService(Mobius::GENERAL_SERVICE)) {
                    charsConnected = connectToCharacteristics(peripheral);
                }
            }
            if (charsConnected) {
                Serial.println(" Successful");
            }
            else {
                // could connect BUT not discover required characteristics
                // disconnect from the device
                Serial.println(" Failed");
                peripheral.disconnect();
            }
        }
        else {
            // didn't connect to  the device
            Serial.println(" Failed");
        }
    }
    // turn off green indicating discovery is complete
    if (greenLed) digitalWrite(greenLed, ledOff);
    return charsConnected;
}
/*!
 * @brief Connect to relevant characteristics
 *
 * Connect to the relevant characteristics on the given 'peripheral' for sending
 * and receiving messages.
 * - REQUEST_CHARACTERISTIC must be found and writable
 * - RESPONSE_CHARACTERISTIC_1 must be found and subscribed to
 * - RESPONSE_CHARACTERISTIC_2 must be found and subscribed to
 *
 * @return true only if all the required characteristics are connected/ready
 */
bool MobiusDevice::connectToCharacteristics(BLEDevice& peripheral) {
    if (debug) {
        Serial.print("connectToCharacteristics() -> peripheral:");
        Serial.println(peripheral.address());
    };
    // assuming peripheral is connected
    // get the "request" characteristic
    _requestChar = peripheral.characteristic(Mobius::REQUEST_CHARACTERISTIC);
    bool hasRequestChar = _requestChar && _requestChar.canWrite();
    // get the "response" characteristics
    BLECharacteristic responseChar1 = peripheral.characteristic(Mobius::RESPONSE_CHARACTERISTIC_1);
    bool hasResponseChar1 = responseChar1 && responseChar1.canSubscribe() && responseChar1.subscribe();
    _responseChar = peripheral.characteristic(Mobius::RESPONSE_CHARACTERISTIC_2);
    bool hasResponseChar2 = _responseChar && _responseChar.canSubscribe() && _responseChar.subscribe();

    if (!hasRequestChar || !hasResponseChar1 || !hasResponseChar2) {
        uint16_t yellow[] = { redLed, greenLed };  //yellow
        blinkLEDs(6, yellow); // ~ 3 seconds
        // reset characteristics to unconnected objects
        _requestChar = BLECharacteristic();
        _responseChar = BLECharacteristic();
    }
    if (debug) {
        Serial.print("connectToCharacteristics() -> ");
        Serial.print("hasRequestChar:");
        Serial.print(hasRequestChar);
        Serial.print(" hasResponseChar1:");
        Serial.print(hasResponseChar1);
        Serial.print(" hasResponseChar2:");
        Serial.println(hasResponseChar1);
    }
    return hasRequestChar && hasResponseChar1 && hasResponseChar2;
}
/*!
 * Send a "set" request with the given 'data' (of size 'length').
 *
 * @return true if verification was requested and the response was valid,
 * or if verification was skipped
 */
bool MobiusDevice::setData(uint8_t* data, uint16_t length, bool doVerification) {
    // build a request to SET data on a device
    uint16_t reqSize;
    uint8_t* req = buildRequest(data, length, Mobius::OP_CODE_SET, 0x0800, reqSize);
    uint16_t resSize;
    uint8_t* res = sendRequest(req, reqSize, resSize);

    bool verified = false;
    // verify the response
    if (0 < resSize && doVerification) {
        verified = responseSuccessful(req, reqSize, res, resSize);
    }
    // cleanup sent request & returned response from memory
    delete[] req;
    delete[] res;

    return verified || !doVerification;
}
/*!
 * Send a "get" request with the given 'data' (of size 'length') and parse
 * out the data portion of the response.
 * Sets the value in the given 'dataSize' address to the data's total size.
 *
 * @return the a pointer to the byte array (data)
 */
uint8_t* MobiusDevice::getData(uint8_t* data, uint16_t length, uint16_t& dataSize) {
    // build a request to GET data on a device
    uint16_t reqSize;
    uint8_t* req = buildRequest(data, length, Mobius::OP_CODE_GET, 0x0000, reqSize);
    uint16_t resSize;
    uint8_t* res = sendRequest(req, reqSize, resSize);
    // current assumes the response is for the current request
    uint8_t* responseData = parseResponseData(res, resSize, dataSize);
    // cleanup sent request & returned response from memory
    delete[] req;
    delete[] res;

    if (debug) {
        Serial.print("getData() -> responseData:");
        for (int i=0; i<dataSize; i++) {
            Serial.print(" 0x");
            Serial.print(responseData[i], HEX);
        }
        Serial.println();
    }
    return responseData;
}
/*!
 * Build a byte array representing a Mobius request message.
 * Sets the value in the given 'requestSize' address to the request's total size.
 *
 * @return a pointer to the byte array (request)
 */
uint8_t* MobiusDevice::buildRequest(uint8_t* data, uint16_t length, uint8_t opCode, uint16_t reserved, uint16_t& requestSize) {
    requestSize = length + 11;
    uint8_t* request = new uint8_t[requestSize];

    // first byte is always 02
    request[0] = 0x02;
    // opGroup
    request[1] = Mobius::OP_GROUP_REQUEST;  // C2CI_Request
    // opCode
    request[2] = opCode;
    // mMessageId
    request[3] = lowByte(_messageId); // little endian
    request[4] = highByte(_messageId);// little endian
    _messageId++;
    // mReserved
    request[5] = highByte(reserved);
    request[6] = lowByte(reserved);
    // data size
    request[7] = lowByte(length); // little endian
    request[8] = highByte(length);// little endian
    // data
    for (uint16_t i = 0; i < length; i++) {
        request[9 + i] = data[i];
    }

    short crc = MobiusCRC::crc16(&request[1], requestSize - 3);
    request[requestSize - 2] = (uint8_t)crc;// lowByte(length)
    request[requestSize - 1] = (uint8_t)(crc >> 8); // highByet(length)

    if (debug) {
        Serial.print("buildRequest() -> request:");
        for (int i=0; i<requestSize; i++) {
            Serial.print(" 0x");
            Serial.print(request[i], HEX);
        }
        Serial.println();
    }

    return request;
}
/*!
 * Writes the given 'request' (of size 'length') to the request characteristic.
 * Sets the value in the given 'responseSize' address to the response's total size.
 * Max response size is currently 255.
 *
 * @return a pointer to the byte array (response)
 */
uint8_t* MobiusDevice::sendRequest(uint8_t* request, uint16_t length, uint16_t& responseSize) {
    if (debug) {
        Serial.print("sendRequest() -> request:");
        for (int i=0; i<length; i++) {
            Serial.print(" 0x");
            Serial.print(request[i], HEX);
        }
        Serial.println();
    }
    // setup response info
    responseSize = 0;
    uint8_t* response = new uint8_t[0];
    // get a copy of the request to write
    uint8_t toSend[length];
    memcpy(toSend, request, length);
    // do the actual writing to the characteristic
    bool sent = _requestChar.writeValue(toSend, length);

    bool received = false;
    // look for a response (should only take one)
    Serial.print("Waiting for response ..");
    for (uint8_t i = 0; sent && !received && i < 5; i++) {
        Serial.print('.');
        uint16_t lightBlue[] = { blueLed, greenLed }; // light blue
        blinkLEDs(1, lightBlue);
        if (_responseChar.valueUpdated()) {
            uint8_t responseValue[255];
            responseSize = _responseChar.readValue(responseValue, sizeof responseValue);
            // cleanup response and copy the data to be returned
            delete[] response;
            response = new uint8_t[responseSize];
            memcpy(response, responseValue, responseSize);
            received = 0 < responseSize;
        }
    }
    Serial.println((received ? " Successful" : " Failed"));
    if (debug) {
        Serial.print("sendRequest() -> response:");
        for (int i=0; i<responseSize; i++) {
            Serial.print(" 0x");
            Serial.print(response[i], HEX);
        }
        Serial.println();
    }

    return response;
}
/*!
 * Parse the response to get extract the data.
 * Sets the value in the given 'dataSize' address to the data's total size.
 *
 * @return a pointer to the byte array (data)
 */
uint8_t* MobiusDevice::parseResponseData(uint8_t* response, uint16_t length, uint16_t& dataSize) {
    // setup default data info
    dataSize = 0;
    uint8_t* data = new uint8_t[0];
    // check response info
    bool isValid = (length > 11);
    isValid = isValid && (0x02 == response[0]);
    isValid = isValid && (Mobius::OP_GROUP_CONFIRM == response[1]);
    if (isValid) {
        // get the data 
        dataSize = (response[8] << 8) + (response[7]);
        // cleanup data and copy the data to be returned
        delete[] data;
        data = new uint8_t[dataSize];
        memcpy(data, &response[9], dataSize);
    }
    if (debug) {
        Serial.print("parseResponseData() -> data:");
        for (int i=0; i< dataSize; i++) {
            Serial.print(" 0x");
            Serial.print(data[i], HEX);
        }
        Serial.println();
    }

    return data;
}
/*!
 * Validate the given 'response' (of size 'resSize') for the given 'request' (of size 'reqSize').
 *
 * @return true only if the response is a success message for the request
 */
bool MobiusDevice::responseSuccessful(uint8_t* request, uint16_t reqSize, uint8_t* response, uint16_t resSize) {
    bool idValid = false;
    bool dataSuccess = false;
    bool lengthsValid = (reqSize > 11) && (resSize > 11);
    if (lengthsValid) {
        // check first 5 bytes (which should match)
        idValid = (request[0] == response[0]);
        idValid = idValid && (response[1] == Mobius::OP_GROUP_CONFIRM); // C2CI_Confirm
        idValid = idValid && (request[2] == response[2]);
        idValid = idValid && (request[3] == response[3]);
        idValid = idValid && (request[4] == response[4]);
        // check the data
        int dataSize = (response[8] << 8) + (response[7]);
        uint8_t resData[dataSize];
        dataSuccess = (3 == dataSize);
        dataSuccess = dataSuccess && (0x00 == response[9]); // all response data starts with 0x00
        for (int i = 0; dataSuccess && i < dataSize - 1; i++) {
            resData[i] = response[10 + i];
            dataSuccess = dataSuccess && response[10 + i] == Mobius::RESPONSE_DATA_SUCCESSFUL[i];
        }
    }
    // check CRC of the response
    // skipping CRC validation for now because it seems to be different
    // Mobius app doesn't seem to be checking either
    //  short crc = crc16(&response[1], resSize-3);
    //  bool crcValid = (response[resSize-2] == (byte) crc) && (response[resSize-1] = (byte) (crc >> 8));
    if (debug) {
        Serial.print("responseSuccessful() -> lengthsValid:");
        Serial.print(lengthsValid);
        Serial.print(" idValid:");
        Serial.print(idValid);
        Serial.print(" dataSuccess:");
        Serial.println(dataSuccess);
    }
    // skipping the CRC validation for now
    return lengthsValid && idValid /*&& crcValid*/ && dataSuccess;
}



/*!
 * @brief Blink LEDs
 *
 * Blink the LED specified by 'led' a total of 'count' times.
 * Each "blink" will last for 500 ms.
 */
void MobiusDevice::blinkLEDs(uint16_t count, uint16_t leds[])
{
    // wait for 250 milliseconds during each on / off phase
    uint8_t delayCount = 250;

    for (short i = 0; i < count; i++) {
        // set the initial start time value
        unsigned long startMillis = millis();
        // turn on all the given LEDs
        for (uint8_t j = 0; j < sizeof leds; j++) {
            if (leds[j]) digitalWrite(leds[j], ledOn);
        }
        // delaying without sleeping
        while (delayCount > (millis() - startMillis)) {}
        // reset the start time value
        startMillis = millis();
        // turn off all the given LEDs
        for (uint8_t j = 0; j < sizeof leds; j++) {
            if (leds[j]) digitalWrite(leds[j], ledOff);
        }
        // delaying without sleeping
        while (delayCount > (millis() - startMillis)) {}
    }
}
