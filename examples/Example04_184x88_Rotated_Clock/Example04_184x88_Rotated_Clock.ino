// Example04 : 184x88 clock emulator using partial updates
//
// Written by P.C. @ SparkFun Electronics, April 2026
//
// This is an experimental library to control SSD1680/1 e-Paper displays via I2C, using a I2C to SPI Bridge.
//
// The I2C SPI Bridge is configured as a I2C peripheral with three registers: Control (Register 0x00),
// Data (Register 0x01) and Reset (Register 0x02).
// All data written to Register 0x00 is bridged to SPI with the D/C# pin held low.
// All data written to Register 0x01 is bridged to SPI with the D/C# pin held high.
// A write to Register 0x02 causes RST to be pulled low briefly.
// I2C reads return bytes containing the e-paper BUSY flag in the LSB.
//
// SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT

#include <SparkFun_SSD168x_I2C_Interface_Library.h> // http://librarymanager/All#SparkFun_SSD168x_I2C_Interface_Library

SSD1680I2C184x88Rotated myDevice;

// Fonts
#include <res/qw_fnt_largenum.h>

// Static char array to hold the previous time
// Load the previous time with zeros for the background / basemap for partial updates
static char previousTime[strlen("HHHH:MM:SS") + 1] = { '0', '0', '0', '0', ':', '0', '0', ':', '0', '0', '\0' };

// Define the start coordinates for displaying the time
const int xStart = 40;
const int yStart = 20;

// Change this to invert the colors
const bool invertColors = true;

void setup()
{
    delay(1000);
    
    // Start serial
    Serial.begin(115200);
    Serial.println("Running OLED example");

    Wire.begin();

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
    myDevice.setFont(QW_FONT_LARGENUM);
    myDevice.text(xStart, yStart, previousTime, invertColors ? COLOR_OFF : COLOR_ON);

    // There's nothing on the screen yet
    // Send the graphics to the device and also set the background for partial updates
    myDevice.displayBackground();

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

    // work out how many and which characters have changed
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
            if (charsChanged[i])
            {
                myDevice.rectangleFill(xStart + i * FONT_LARGENUM_WIDTH, yStart,
                                FONT_LARGENUM_WIDTH, FONT_LARGENUM_HEIGHT, invertColors ? COLOR_ON : COLOR_OFF);
                char newChar[2];
                sprintf(newChar, "%c", theTime[i]);
                myDevice.text(xStart + i * FONT_LARGENUM_WIDTH, yStart, newChar, invertColors ? COLOR_OFF : COLOR_ON);
            }
        }

        // if only 1 character has changed, do a partial update,
        // otherside do a full update and update the background

        bool doPartial = numCharsChanged <= 1;

        //myDevice.display(doPartial, !doPartial);
        if (doPartial)
            myDevice.displayPartial();
        else
            myDevice.displayBackground();

        // Wait for display to update
        do {
            delay(10); // Don't pound the I2C bus too hard
        } while (myDevice.isBusy());

        myDevice.deepSleep();

        // update previousTime
        strcpy(previousTime, theTime);
    }
}
