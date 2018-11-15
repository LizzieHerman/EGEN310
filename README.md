# EGEN310
EGEN 310 Group F.6 Fall Semester 2018 Cat's Conundrum


### To Download the App to your phone:
  1) Find security settings on phone and change to allow apps downloaded from unknown sources
  2) Download the `.apk` fie to your phone (**_NOT_** the `.aia` file, this is the code/data for App Inventor)
  3) Go to files on your phone, under downloaded click this attachment
  4) Download and find in your app section under Control_Gyro
  5) Time to Connect!

### Setting Up the BLE Connection with The Microcontroller:
  1) Power the Microcontroller
  2) Open the app
  3) Allow/Turn-on Bluetooth (If not already done)
  4) Click the Bluetooth button in the bottom left hand corner
  5) Select the Microcontroller - A new list will apear with a fair amount of numbers
  6) Choose 6e400001... (Sets the Service ID for the Adafruit Microcontroller we are currently using) - A new list will apear with a fair amount of numbers
  7) Choose 6E400002... (Allows writing for the Adafruit Microcontroller we are currently using)
  8) Your all set and ready to send drive data!

### Curent App data as of V2.2
  * After the START buton is clicked the gyroscope is turned on and will collect/send any tilts to the phone right or left above a certain threshold (to slow data transfer rate)
  * The gyroscope changes the turn angle meter and will send data to the Microcontroller in the range of 0 (full left) to 180 (full right)

  * The bar on the right hand side is a slider that will change the speed meter and send data to the Microcontroller in the range of 0 (bottom of screen) to 180 (top of screen)

  * The square in the top left corner of the screen registers if BLE is connected (blue) dissconected (red) or not yet set up (yellow/orange)

  * After the start button is clicked it changes to a STOP button which, when clicked, will reset both speed and turn meters and send out the signal 90 (a full stop) for both

  * The x in the top corner disconects Bluetooth and closes the app
