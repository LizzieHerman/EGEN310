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

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

Adafruit_Crickit crickit;

seesaw_Servo left(&crickit); //left motor needs to be the opposite
seesaw_Servo right(&crickit);
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
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit App Controller Example"));
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

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
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
  // Buttons
  if (packetbuffer[1] == 'B') {
    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
    Serial.print ("Button "); Serial.print(buttnum);
    if (pressed) {
      switch(buttnum){
        case 5: //up
          Serial.println(" pressed.");
          if(velocity >= 10){ // check if the car is at top speed
            Serial.println("Car cannot go any faster");
          } else if(leftvel <= 0 || rightvel >= 180){ // check if either left or right motor is at top speed
            Serial.println("Car cannot increase speed without changing turning radius");
          } else {
            leftvel -= 9; // increase speed by 1/10 of max
            rightvel += 9;
            if(leftvel < 0){ // check if the increase was too much
              rightvel += leftvel; // preserve the speed differential between the two motors
            }
            if(rightvel > 180){ // check if the increase was too much
              leftvel += (rightvel - 180); // preserve the speed differential between the two motors
            }
            if(leftvel < 0) leftvel = 0;
            if(rightvel < 0) rightvel = 0;
            if(leftvel > 180) leftvel = 180;
            if(rightvel > 180) rightvel = 180;
            if(leftvel == 90 || rightvel == 90||(leftvel > 90 && rightvel > 90)||(leftvel < 90 && rightvel < 90)){ // this means that there is no forward/ backward momentum
              velocity = 0; // the car is just turning in place
            } else {
              velocity = ((((180 - leftvel) + rightvel) / 2.0) - 90) / 9;
            }
            left.write(leftvel);
            right.write(rightvel);
            Serial.print("Velocity now "); Serial.println(velocity, 2);
          }
          break;
        case 6: //down
          Serial.println(" pressed.");
          if((velocity) <= -10){ // check if the car is at top reverse speed
            Serial.println("Car cannot reverse any faster");
          } else if(leftvel >= 180 || rightvel <= 0){ // check if either left or right motor is at reverse speed
            Serial.println("Car cannot increase speed without changing turning radius");
          } else {
            leftvel += 9; // decrease speed by 1/10 of max
            rightvel -= 9;
            if(leftvel > 180){ // check if the decrease was too much
              rightvel += (180 - leftvel); // preserve the speed differential between the two motors
            }
            if(rightvel < 0){ // check if the decrease was too much
              leftvel += rightvel; // preserve the speed differential between the two motors
            }
            if(leftvel < 0) leftvel = 0;
            if(rightvel < 0) rightvel = 0;
            if(leftvel > 180) leftvel = 180;
            if(rightvel > 180) rightvel = 180;
            if(leftvel == 90 || rightvel == 90||(leftvel > 90 && rightvel > 90)||(leftvel < 90 && rightvel < 90)){ // this means that there is no forward/ backward momentum
              velocity = 0; // the car is just turning in place
            } else {
              velocity = ((((180 - leftvel) + rightvel) / 2.0) - 90) / 9;
            }
            left.write(leftvel);
            right.write(rightvel);
            Serial.print("Velocity now "); Serial.println(velocity, 2);
          }
          break;
        case 7: //left
          Serial.println(" pressed.");
          if(leftvel > 8){
            leftvel -= 9;
          } else if (leftvel <= 0){
            rightvel -= 9;
          } else {
            leftvel -= 9;
            rightvel += leftvel;
          }
          if(leftvel < 0) leftvel = 0;
          if(rightvel < 0) rightvel = 0;
          if(leftvel > 180) leftvel = 180;
          if(rightvel > 180) rightvel = 180;
          if(leftvel == 90 || rightvel == 90||(leftvel > 90 && rightvel > 90)||(leftvel < 90 && rightvel < 90)){ // this means that there is no forward/ backward momentum
            velocity = 0; // the car is just turning in place
          } else {
            velocity = ((((180 - leftvel) + rightvel) / 2.0) - 90) / 9; // 
          }
          angle = (rightvel - (180 - leftvel)) / 9.0;
          left.write(leftvel);
          right.write(rightvel);
          Serial.print("Velocity now "); Serial.println(velocity, 2);
          Serial.print("Wheel angle now "); Serial.println(angle, 2);
          break;
        case 8: //right
          Serial.println(" pressed.");
          if(rightvel < 172){
            rightvel += 9;
          } else if (rightvel >= 180){
            leftvel += 9;
          } else {
            rightvel += 9;
            leftvel += (rightvel - 180);
          }
          if(leftvel < 0) leftvel = 0;
          if(rightvel < 0) rightvel = 0;
          if(leftvel > 180) leftvel = 180;
          if(rightvel > 180) rightvel = 180;
          if(leftvel == 90 || rightvel == 90||(leftvel > 90 && rightvel > 90)||(leftvel < 90 && rightvel < 90)){ // this means that there is no forward/ backward momentum
            velocity = 0; // the car is just turning in place
          } else {
            velocity = ((((180 - leftvel) + rightvel) / 2.0) - 90) / 9; // 
          }
          angle = ((rightvel - 90) - (90 - leftvel)) / 9.0;
          left.write(leftvel);
          right.write(rightvel);
          Serial.print("Velocity now "); Serial.println(velocity, 2);
          Serial.print("Wheel angle now "); Serial.println(angle, 2);
          break;
        case 1: //1
          Serial.println(" pressed.");
          velocity = 0;
          angle = 0;
          leftvel = 90;
          rightvel = 90;
          left.write(leftvel);
          right.write(rightvel);
          Serial.print("\nCar stopped \nVelocity now "); Serial.println(velocity, 2);
          break;
        case 2: //2
          Serial.println(" pressed.");
          angle = 0;
          if(velocity < -10) velocity = -10;
          if(velocity > 10) velocity = 10;
          rightvel = roundf((velocity * 9) + 90);
          leftvel = (180 - rightvel);
          velocity = (rightvel - 90) / 9.0;
          left.write(leftvel);
          right.write(rightvel);
          Serial.print("\nCar straightened out \nWheel angle now "); Serial.println(angle, 2);
          Serial.print("Velocity now "); Serial.println(velocity, 2);
          break;
        case 3: 
          Serial.println(" pressed.");
          digitalWrite(13, HIGH);
          break;
        default:
          Serial.println(" has not been assigned a task.");
      }
    } else {
      Serial.println(" released");
      if(buttnum == 3){
        digitalWrite(13, LOW);
      }
    }
  }
}
