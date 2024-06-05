## PETnessPal

This is a project for managing pet feeding schedules and weight tracking using an ESP32, HX711 ADC, and Firebase Realtime Database.

## Description

This is the codebase for PETnessPal App specifically focuses on the ESP32 that uses firebase library to communicate with my web application

## Getting Started

To get started with the PETnessPal App on ESP32, follow these steps:

1. Clone the repository:

   ```bash
   git clone https://github.com/ProgrammerJM/PETnessPal_App_ESP32.git
   ```

2. Install the required libraries. Open the PlatformIO project in your preferred IDE and navigate to the project directory. Run the following commands in the terminal:

   ```bash
   platformio lib install --save mobizt/Firebase@^4.4.14
   platformio lib install --save bblanchon/ArduinoJson@^7.0.4
   platformio lib install --save olkal/HX711_ADC@^1.2.12
   ```

3. Open the `main.cpp` file in your IDE and make any necessary configuration changes.

4. Connect your ESP32 board to your computer.

5. Build and upload the code to your ESP32 board.

6. Once the code is uploaded, open the serial monitor to view the output.

7. The PETnessPal App is now ready to use with your ESP32 board.

## Dependencies

Library used:

- mobizt/Firebase Arduino Client Library for ESP8266 and ESP32 @ ^4.4.14
- bblanchon/ArduinoJson @ ^7.0.4
- olkal/HX711_ADC @ ^1.2.12

## Authors

Tizado, John Mark G. [www.linkedin.com/johnmarktizado](https://ph.linkedin.com/in/johnmarktizado)

## Version History

- 0.1
  - Initial Release
