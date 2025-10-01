# I2C Scanner for ArduTester

## Overview
This modification adds an I2C address scanner to the ArduTester project. The scanner will detect I2C devices connected to the Arduino Nano's A4 (SDA) and A5 (SCL) pins and display their addresses on the LCD.

## Features
- Scans I2C addresses from 0x08 to 0x77 (standard I2C address range)
- Displays found device addresses in hexadecimal format
- Shows device count and pagination for multiple devices
- Uses bit-bang I2C implementation (no Wire library dependency)
- Integrates seamlessly with the existing menu system

## Usage
1. Power on the ArduTester
2. Hold the test button for more than 3 seconds to enter the menu
3. Navigate through the menu until you find "I2C-Scanner"
4. Press the test button to select
5. The scanner will automatically scan for devices and display results

## Display Format
- Line 1: Shows device count and page info (e.g., "2 I2C found P1/1")
- Line 2-3: Shows I2C addresses in hexadecimal format (e.g., "0x3C 0x50 0x68")
- If no devices found: Shows "No I2C devices found!"

## Hardware Connections
- Connect I2C devices to Arduino Nano pins:
  - SDA → A4 (PC4)
  - SCL → A5 (PC5)
  - VCC → 5V or 3.3V (depending on device)
  - GND → GND

## Implementation Details
The I2C scanner uses a simple bit-bang I2C implementation that:
- Sets A4 and A5 as inputs with pull-up resistors
- Implements standard I2C start/stop conditions
- Sends device address with write bit
- Checks for ACK response to determine device presence

## Files Modified/Added
1. `I2CScanner.ino` - New file containing the scanner implementation
2. `function_menu.ino` - Added new mode and menu integration
3. `tt_function.h` - Added function declarations
4. `Transistortester.h` - Added string constants
5. `ArduTester_1_13.ino` - Minor includes (removed Wire.h as not needed)

## Testing
To test the I2C scanner:
1. Connect a known I2C device (e.g., OLED display at 0x3C, RTC at 0x68)
2. Run the scanner and verify the device address appears
3. Disconnect the device and verify it no longer appears

## Troubleshooting
- If no devices are detected, check:
  - Wiring connections (SDA to A4, SCL to A5)
  - Pull-up resistors (typically 4.7kΩ on SDA and SCL lines)
  - Device power supply
  - Device I2C address (some devices have configurable addresses)

## Common I2C Device Addresses
- 0x3C, 0x3D: OLED displays (SSD1306)
- 0x48-0x4F: ADS1115 ADC, PCF8591
- 0x50-0x57: EEPROM (24C32, 24C64, etc.)
- 0x68: DS1307, DS3231 RTC
- 0x76, 0x77: BMP280, BME280 pressure sensors

## Notes
- The scanner uses a 5μs delay between I2C operations for timing
- Maximum 112 I2C addresses can be stored (though practically much fewer will be found)
- The implementation is compatible with both 3.3V and 5V I2C devices
- Pull-up resistors are enabled on A4 and A5 during scanning