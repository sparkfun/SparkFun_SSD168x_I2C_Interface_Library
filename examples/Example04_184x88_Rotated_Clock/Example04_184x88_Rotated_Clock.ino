// Example04 : 184x88 clock emulator using partial updates
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
// SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT

#include <SparkFun_SSD168x_I2C_Interface_Library.h> // http://librarymanager/All#SparkFun_SSD168x_I2C_Interface_Library

SSD1680I2C184x88Rotated myDevice;

// Fonts
#include <res/qw_ep_fnt_largenum.h>

// Static char array to hold the previous time
// Load the previous time with zeros for the background / basemap for partial updates
static char previousTime[strlen("HHHH:MM:SS") + 1] = { '0', '0', '0', '0', ':', '0', '0', ':', '0', '0', '\0' };

// Define the start coordinates for displaying the time
const int xStart = 40;
const int yStart = 20;

// Change this to invert the colors
const bool invertColors = false;

// This defines how many digit changes trigger a full update
typedef enum {
    EVERY_SECOND = 1,
    EVERY_10_SECONDS,
    EVERY_MINUTE,
    EVERY_10_MINUTES,
    EVERY_HOUR,
    EVERY_10_HOURS,
} numCharsChangedLimit_e;

// Change numCharsChangedLimit to change how often a full update is performed
const numCharsChangedLimit_e numCharsChangedLimit = EVERY_MINUTE;

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

    // Fill the whole screen
    myDevice.rectangleFill(0, 0, myDevice.getWidth(), myDevice.getHeight(), invertColors ? COLOR_ON : COLOR_OFF);

    // Fill a rectangle on the screen that has a 4 pixel border
    myDevice.rectangleFill(4, 4, myDevice.getWidth() - 8, myDevice.getHeight() - 8, invertColors ? COLOR_OFF : COLOR_ON);

    // Fill a rectangle within that, to leave an 4 pixel frame
    myDevice.rectangleFill(8, 8, myDevice.getWidth() - 16, myDevice.getHeight() - 16, invertColors ? COLOR_ON : COLOR_OFF);

    // Add the 0000:00:00 held in previousTime - this becomes the background for partial updates
    myDevice.setFont(QW_EP_FONT_LARGENUM);
    myDevice.text(xStart, yStart, previousTime, invertColors ? COLOR_OFF : COLOR_ON);

    // There's nothing on the screen yet
    // Send the graphics to the device and also set the background for partial updates
    myDevice.display();

    // Wait for display to update
    while (myDevice.isBusy())
        delay(10); // Don't pound the I2C bus too hard    
}

void loop()
{
    // char array to hold the time
    char theTime[strlen("HHHH:MM:SS") + 1];

    // split millis into hours, mins and secs
    int hh,mm,ss;
    ss = millis() / 1000;
    hh = ss / 3600;
    ss -= hh * 3600;
    mm = ss / 60;
    ss -= mm * 60;

    // print into theTime
    sprintf(theTime, "%04d:%02d:%02d", hh, mm, ss);

    // Work out how many and which characters have changed
    int numCharsChanged = 0;
    bool charsChanged[strlen("HHHH:MM:SS")];
    for (int i = 0; i < strlen("HHHH:MM:SS"); i++)
    {
        if (theTime[i] != previousTime[i])
        {
            charsChanged[i] = true;
            numCharsChanged++;
        }
        else
            charsChanged[i] = false;
    }

    // if any characters have changed
    if (numCharsChanged > 0)
    {
        Serial.println(theTime);

        // for each character that has changed, erase it and update it
        for (int i = 0; i < strlen("HHHH:MM:SS"); i++)
        {
            if (charsChanged[i]) // Comment this if statement if desired to always update all the characters
            {
                myDevice.rectangleFill(xStart + i * FONT_LARGENUM_WIDTH, yStart,
                                FONT_LARGENUM_WIDTH, FONT_LARGENUM_HEIGHT, invertColors ? COLOR_ON : COLOR_OFF);
                char newChar[2];
                sprintf(newChar, "%c", theTime[i]);
                myDevice.text(xStart + i * FONT_LARGENUM_WIDTH, yStart, newChar, invertColors ? COLOR_OFF : COLOR_ON);
            }
        }

        // Do a partial update if numCharsChanged is less than numCharsChangedLimit
        bool partial = numCharsChanged < numCharsChangedLimit;
        bool dirtyOnly = true; // dirtyOnly defaults to true

        if (!partial)
        {
            dirtyOnly = false;
            Serial.println("Performing full update - sending all pixels");
        }

        myDevice.display(partial, dirtyOnly);

        // Wait for display to update
        do {
            delay(10); // Don't pound the I2C bus too hard
        } while (myDevice.isBusy());

        myDevice.deepSleep();

        // update previousTime
        strcpy(previousTime, theTime);
    }
}
