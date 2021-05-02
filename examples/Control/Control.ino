/*!
 * Control a Mobius Device
 *
 * This example shows how a Mobius device may be controlled with an analog signal.
 * First this will scans for BLE enabled Mobius devices (expecting just one). Once
 * the device is discovered this checks the analog PIN (A0) every 2 seconds for the
 * current state. When a new state is detected this will connect to the Mobius
 * device and set the scene corresponding to the state.
 * 
 * The circuit:
 * - Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board
 * 
 * This example code is released into the public domain.
 */

#include <ArduinoBLE.h>
#include <MobiusBLE.h>

// define the LED pin numbers for Nano 33 BLE board
const int RED_LED = 22;
const int BLUE_LED = 24;
const int GREEN_LED = 23;

// define the ON and OFF values for LEDs on Nano 33 BLE board
const int LED_ON = LOW;
const int LED_OFF = HIGH;

// define analog pin which will get the divided 0-10V signal
const int SENSOR_PIN = A0;

// define a variable for holding the state, initialized to neutral/normal
byte currentState = 0;
// define a variable for the MobiusDevice to be controlled
MobiusDevice pump;

/*!
 * Main Setup method
 */
void setup() {
  // connect the serial port for logs
  Serial.begin(9600);
  while (!Serial);
  
  // allow the ADC to be as precise a possible
  analogReadResolution(12);
  
 // initialize the digital LED pins as outputs
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  // turn off all LEDs
  digitalWrite(RED_LED, LED_OFF);
  digitalWrite(BLUE_LED, LED_OFF);
  digitalWrite(GREEN_LED, LED_OFF);

  // begin BLE initialization
  if (!BLE.begin()) {
   Serial.println("Failed to initialize BLE");
    while (1) { 
      // do not continue without BLE
    }
  }


  // enable LEDs to be used by the MobiusDevices
  MobiusDevice::redLed = RED_LED;
  MobiusDevice::blueLed = BLUE_LED;
  MobiusDevice::greenLed = GREEN_LED;

  // define a String buffer to hold found Mobius device addresses
  String addressBuffer[5];

  // find nearby Mobius devices
  int count = 0;
  while (!count) {
    count = MobiusDevice::scanForMobiusDevices(addressBuffer);
  }

  // check all the devices were found
  int expectedDevices = 1;
  if (count != expectedDevices) {
    Serial.println("Failed find all the Mobius devices");
  }
  
  // initialize the device to be contolled
  pump = MobiusDevice(addressBuffer[0]);
  
  // reset the BLE initialization
  BLE.begin();
}


/*!
 * Main Loop method
 */
void loop() {
  // get the analog state
  byte newState = analogState();

  if (currentState != newState) {
    // now in a different state, update a maybe do something
    if (1 == newState && pump.connect()) {
      Serial.println("Feed Mode");
      // new state is the feed state AND the device is connected
      if(pump.setFeedScene()) {
        // update the current state so not to re-enter feed state next loop
        currentState = newState;
      }
      // disconnect from the device
      pump.disconnect();
    } else if (2 == newState && pump.connect()) {
      Serial.println("Maintenance Mode");
      // new state is the maintenance state AND the device is connected
      // set the sceneId to the custom/unique ID
      uint16_t sceneId = 1234;
      if(pump.setScene(sceneId)) {
        // update the current state so not to re-enter maintenance state next loop
        currentState = newState;
      }
      // disconnect from the device
      pump.disconnect();
    } else {
      // update the current state
      currentState = newState;
    }
  }
  
  // wait/sleep for 2 seconds before checking the state again
  delay(2000);
}

/*!
 * Determines the current "state" from the monitored SENSOR_PIN.
 * Possible state values:
 *   0 : neutral or normal
 *   1 : feed mode state (~5V)
 *   2 : maintenance mode state (~2V)
 */
byte analogState() {
   // read the ADC value (0 - 4095)
  int sensorValue = analogRead(SENSOR_PIN);
  // calculate voltage, assume a voltage divider is in place
  // dropping a 10V max to a board acceptable 3.3V 
  float voltage = sensorValue * (10.0 / 4095.0);
  Serial.print("Input Voltage: ");
  Serial.print(voltage, 5);

  // default state is "neutral"
  byte state = 0;
  if (4.75 <= voltage && voltage <= 5.26) {
    // voltage is within the 5V range
    state = 1;
  } else if (1.75 <= voltage && voltage <= 2.26) {
    // voltage is within the 2V range
    state = 2;
  }
  Serial.print("\tstate: ");
  Serial.println(state);
  return state;
}

