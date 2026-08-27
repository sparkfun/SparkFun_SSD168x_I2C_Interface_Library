// Example08 : 184x88 testing partial updates
//
// Written by P.C. @ SparkFun Electronics, April 2026
//
// This is a library to control SSD1680/1 e-Paper displays via I2C, using a I2C to SPI Bridge.
//
// The I2C SPI Bridge is configured as a I2C peripheral with five registers:
// Single Control (Register 0x00), Control (Register 0x01), Data (Register 0x02), Final Data (Register 0x03) and Reset (Register 0x04).
// A single control byte written to Register 0x00 is bridged to SPI with the D/C# pin held low. CS returns high after the write.
// All data written to Register 0x01 is bridged to SPI with the D/C# pin held low. CS remains low after the write.
// All data written to Register 0x02 is bridged to SPI with the D/C# pin held high. CS remains low after the write.
// All data written to Register 0x03 is bridged to SPI with the D/C# pin held high. CS returns high after the write.
// A write to Register 0x04 causes RST to be pulled low briefly.
// I2C reads return bytes containing the e-paper BUSY flag in the LSB.
//
//
// Notes on the GoodDisplay GDEM0097T61:
//
// Command 0x2D (Read SSD1680 OTP Register) returns:
// VCOM OTP Selection (Command 0x37, Byte A):  0x00
// VCOM Register (Command 0x2C):               0x00
// Display Mode (Command 0x37, Bytes B-F):     0x00 0x01 0x00 0x00 0x40
// Waveform version (Command 0x37, Bytes G-J): 0x00 0x00 0x00 0x00
//
// Mode 2 ping-pong is enabled: bit F[6] is set
//
// Command 0x2E (Read SSD1680 User ID) returns: 0xCA 0xFE 0x00 0x16 0x80 0x00 0x55 0x01 0x00 0xDB
//
// The ping-pong causes some unexpected results with partial updates
// If we want to move or scroll a digit by: erasing it with a single space; and adding the digit at a new position
// We need to perform the erase-and-add twice, so that both ahlves of the SSD1680 RAM are updated
//
// SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT

#include <SparkFun_SSD168x_I2C_Interface_Library.h> // http://librarymanager/All#SparkFun_SSD168x_I2C_Interface_Library

SSD1680I2C184x88Rotated myDevice;

// Fonts
#include <res/qw_ep_fnt_largenum.h> // 12x48

// Define the start coordinates for displaying the digits
const int xStart = 27;
const int yStart = 20;

// Change this to invert the colors
// The ghosting from the partial updates is more evident when inverted
const bool invertColors = false;

// Change this to false to display stripes instead of digits
const bool displayDigits = true;

const int numDigits = 11; // Print 0123456789: using partial updates
const int numLoops = 3; // Print digits this many times before doing a full refresh
const int numWrites = 1; // Write each partial update this many times - useful to check ghosting

// Adjust these values according to your configuration
//------------------------------------------------------------------------------

// Pre-defined boards - comment / uncomment as needed:
//#define FACET_FP
//#define POSTCARD
#define ESP32_THING_PLUS_C

#ifdef  FACET_FP

// https://www.sparkfun.com/sparkpnt-fp-no-gnss-receiver.html
int pin_SDA = 15;
int pin_SCL = 4;
const char * platform = "SparkPNT FP";

#else   // FACET_FP
#ifdef  POSTCARD

// https://www.sparkfun.com/sparkfun-rtk-postcard.html
int pin_SDA = 7;
int pin_SCL = 20;
const char * platform = "SparkFun RTK Postcard";

#else   // POSTCARD
#ifdef ESP32_THING_PLUS_C

// https://www.sparkfun.com/sparkfun-thing-plus-esp32-wroom-usb-c.html
int pin_SDA = 21;
int pin_SCL = 22;
const char * platform = "SparkFun ESP32 Thing Plus C";

#else  // ESP32_THING_PLUS_C

// https://www.sparkfun.com/sparkfun-iot-redboard-esp32-development-board.html
int pin_SDA = 21;
int pin_SCL = 22;
const char * platform = "SparkFun IoT Redboard";

#endif  // ESP32_THING_PLUS_C
#endif  // POSTCARD
#endif  // FACET_FP

void setup()
{
    delay(1000);
    
    // Start serial
    Serial.begin(115200);
    Serial.printf("Running SSD168x example on %s\r\n", platform);

    Wire.begin(pin_SDA, pin_SCL);

    // Initalize the device and related graphics system
    if (myDevice.begin() == false)
    {
        Serial.println("Device begin failed. Freezing...");
        while (true)
            ;
    }
    Serial.println("Begin success");

    myDevice.setFont(QW_EP_FONT_LARGENUM);
}

void loop()
{
    static int digitCount = 0;
    static int loopCount = 0;

    if ((digitCount == 0) && (loopCount == 0))
    {
        // Fill the whole screen
        myDevice.rectangleFill(0, 0, myDevice.getWidth(), myDevice.getHeight(), invertColors ? COLOR_ON : COLOR_OFF);

        // Send the graphics to the device and also set the background for partial updates
        myDevice.display();

        // Wait for display to update
        do {
            delay(10); // Don't pound the I2C bus too hard
        } while (myDevice.isBusy());
    }

    // Digits 0123456789:
    if (displayDigits)
    {
        for (int w = 0; w < numWrites; w++) // Do each partial update this many times
        {
            // Erase the previous digit
            if ((digitCount == 0) && (loopCount == 0))
            {
                // Nothing to do
            }
            else
            {
                int i = digitCount - 1;
                if (i < 0)
                    i = numDigits - 1;
    
                myDevice.rectangleFill(xStart + i * FONT_LARGENUM_WIDTH, yStart,
                                       FONT_LARGENUM_WIDTH, FONT_LARGENUM_HEIGHT,
                                       invertColors ? COLOR_ON : COLOR_OFF);
            }

            // Write the new digit
            char newChar[2];
            sprintf(newChar, "%c", '0' + digitCount);
            myDevice.text(xStart + digitCount * FONT_LARGENUM_WIDTH, yStart, newChar,
                          invertColors ? COLOR_OFF : COLOR_ON);

            // Partial update
            myDevice.display(true);

            // Wait for display to update
            do {
                delay(10); // Don't pound the I2C bus too hard
            } while (myDevice.isBusy());
        }
    }

    // Stripes : 11 8-pixel stripes, matches the display width of 88 pixels in bytes
    else
    {
        for (int w = 0; w < numWrites; w++) // Do each partial update this many times
        {
            // Erase the previous stripe
            if ((digitCount == 0) && (loopCount == 0))
            {
                // Nothing to do
            }
            else
            {
                int i = digitCount - 1;
                if (i < 0)
                    i = numDigits - 1;
    
                myDevice.rectangleFill(0, i * 8, 184, 8, invertColors ? COLOR_ON : COLOR_OFF);
            }

            // Write the new stripe
            myDevice.rectangleFill(0, digitCount * 8, 184, 8, invertColors ? COLOR_OFF : COLOR_ON);

            // Partial update
            myDevice.display(true);

            // Wait for display to update
            do {
                delay(10); // Don't pound the I2C bus too hard
            } while (myDevice.isBusy());
        }
    }

    // Increment the counters
    digitCount++;
    digitCount %= numDigits;
    if (digitCount == 0)
    {
        loopCount++;
        loopCount %= numLoops;
    }

    // Put display into deep sleep - just to prove we can
    myDevice.deepSleep();

    // Delay
    delay(1000);
}