/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules
 Pick one up today in the adafruit shop!
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#include "Adafruit_Crickit.h"
#include "seesaw_servo.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/*=========================================================================
    APPLICATION SETTINGS
    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
//float parsefloat(uint8_t *buffer);
//void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

Adafruit_Crickit crickit;

seesaw_Servo left(&crickit);
seesaw_Servo right(&crickit); // right servo rotation reversed
int leftvel = 90; // range 0 - 180; 0: full speed backward, 90: stop, 180: full speed forward
int rightvel = 90; // Adafruit's Continuous Rotation Servo PID: 154
float velocity = 0; // the average velocity of both motors
float angle = 0;


/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(115200);
  Serial.println(F("EGEN 310 Group F6 App Testing/Initialization"));
  Serial.println(F("-----------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use App created with MIT App Inventor to connect."));  
  Serial.println();
  
  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));

    if(!crickit.begin()){
    Serial.println("ERROR!");
    while(1);
  }
  else Serial.println("Crickit started");

  left.attach(CRICKIT_SERVO1);  // attaches the servo to CRICKIT_SERVO1 pin
  if(left.attached()){
    Serial.println("Left Servo Attached.");
    left.write(90);
  } else Serial.println("Left Servo Not Attached.");
  right.attach(CRICKIT_SERVO4);  // attaches the servo to CRICKIT_SERVO4 pin
  if(right.attached()){
    Serial.println("Right Servo Attached.");
    right.write(90);
  } else Serial.println("Right Servo Not Attached.");
}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

  /* Got a packet! */

  // if packet = 181 turn change
  if (packetbuffer[0] == 181){
    angle = packetbuffer[1];
    angle -= 90;
    Serial.print("Turn Angle recieved "); Serial.println(angle, 2);
    if(angle < 0){
      leftvel -= roundf(angle);
      if(leftvel > 180){
        rightvel += (leftvel - 180);
      }
    } else if(angle > 0){
      rightvel -= roundf(angle);
      if(rightvel < 0){
        leftvel += rightvel;
      }
    }
  }
  // if packet = 182 speed change
  if (packetbuffer[0] == 182){
    velocity = packetbuffer[1];
    velocity -= 90;
    Serial.print("Velocity recieved "); Serial.println(velocity, 2);
    if(angle < 0){
      leftvel = 90 - roundf(angle);
      rightvel = 90;
    } else if(angle > 0){
      rightvel = 90 - roundf(angle);
      leftvel = 90;
    }
    leftvel += roundf(velocity);
    rightvel -= roundf(velocity);
    if(leftvel < 0) rightvel += leftvel;
    if(rightvel < 0) leftvel += rightvel;
    if(leftvel > 180) rightvel += (leftvel - 180);
    if(rightvel > 180) leftvel += (rightvel - 180);
  }
  if(leftvel < 0) leftvel = 0;
  if(rightvel < 0) rightvel = 0;
  if(leftvel > 180) leftvel = 180;
  if(rightvel > 180) rightvel = 180;
  if(leftvel == 90 || rightvel == 90||(leftvel < 90 && rightvel < 90)||(leftvel > 90 && rightvel > 90)){ // this means that there is no forward/ backward momentum
    velocity = 0; // the car is just turning in place
  } else {
    velocity = ((leftvel + (180 - rightvel)) / 2.0) - 90; // 
  }
  angle = (90 - rightvel) - (leftvel - 90);
  left.write(leftvel);
  right.write(rightvel);
  Serial.print("Velocity now "); Serial.println(velocity, 2);
  Serial.print("Wheel angle now "); Serial.println(angle, 2);
  
}
