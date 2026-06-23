// Example06 : RTK display emulator (184x88) using partial updates
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
#include <res/qw_ep_fnt_5x7.h>
#include <res/qw_ep_fnt_8x16.h>

// Icons
#include "icons.h"

// Change this to invert the colors
const bool invertColors = false;

// Top-Left corner
const int startX = 28;
const int startY = 12;

void setup()
{
    delay(1000);
    
    // Start serial
    Serial.begin(115200);
    Serial.println("Running OLED example");

    Wire.begin();
    Wire.setClock(400000);

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

    // Add the BT MAC top right
    myDevice.setFont(QW_EP_FONT_5X7);
    myDevice.text( startX + 0, startY + 0, "A4DA", invertColors ? COLOR_OFF : COLOR_ON);

    // More text
    myDevice.text( startX + 0, startY + 56, "192.168.1.100", invertColors ? COLOR_OFF : COLOR_ON);

    // Add the WiFi3 icon
    myDevice.setDrawMode( invertColors ? grEpROPNotCopy : grEpROPCopy );
    myDevice.bitmap( startX + 34, startY + 0, (uint8_t *)WiFi_Symbol_3, WiFi_Symbol_Width, WiFi_Symbol_Height );

    // More icons
    myDevice.bitmap( startX + 108, startY + 0, (uint8_t *)Battery_3, Battery_Width, Battery_Height );
    myDevice.bitmap( startX + 0, startY + 26, (uint8_t *)CrossHairDual, CrossHairDual_Width, CrossHairDual_Height );
    myDevice.bitmap( startX + 74, startY + 26, (uint8_t *)SIV_Antenna_PPP, SIV_Antenna_PPP_Width, SIV_Antenna_PPP_Height );
    myDevice.bitmap( startX + 96, startY + 50, (uint8_t *)Corr_TCP_Icon, Corr_Icon_Width, Corr_Icon_Height );
    //myDevice.bitmap( startX + , startY + , (uint8_t *), _Width, _Height );

    // There's nothing on the screen yet
    // Send the graphics to the device and also set the background for partial updates
    myDevice.displayBackground();

    // Wait for display to update
    while (myDevice.isBusy())
        delay(10); // Don't pound the I2C bus too hard    
}

void loop()
{
    static int loopCount = 0;
    const int fullUpdateEvery = 20;
    char theText[8];

    // Add the SIV and HPA
    
    if (invertColors)
        myDevice.setDrawMode( invertColors ? grEpROPCopy : grEpROPNotCopy );

    myDevice.setFont(QW_EP_FONT_8X16);

    myDevice.rectangleFill( startX + 16, startY + 27, 5 * 8, 16, invertColors ? COLOR_ON : COLOR_OFF );
    sprintf(theText, ":.%03d", loopCount % 1000);
    myDevice.text( startX + 16, startY + 27, theText, invertColors ? COLOR_OFF : COLOR_ON);

    myDevice.rectangleFill( startX + 90, startY + 26, 3 * 8, 16, invertColors ? COLOR_ON : COLOR_OFF );
    sprintf(theText, ":%d", loopCount % 100);
    myDevice.text( startX + 90, startY + 26, theText, invertColors ? COLOR_OFF : COLOR_ON);

    if (loopCount % 2 == 1)
        myDevice.rectangleFill( startX + 74, startY + 0, DownloadArrow_Width, DownloadArrow_Height, invertColors ? COLOR_ON : COLOR_OFF );

    if (invertColors)
        myDevice.setDrawMode( invertColors ? grEpROPNotCopy : grEpROPCopy );

    // Blink the DownloadArrow
    if (loopCount % 2 == 0)
        myDevice.bitmap( startX + 74, startY + 0, (uint8_t *)DownloadArrow, DownloadArrow_Width, DownloadArrow_Height );

    // Update the logging icon
    if (loopCount % 4 == 0)
        myDevice.bitmap( startX + 119, startY + 52, (uint8_t *)Logging_0, Logging_Width, Logging_Height );
    else if (loopCount % 4 == 1)
        myDevice.bitmap( startX + 119, startY + 52, (uint8_t *)Logging_1, Logging_Width, Logging_Height );
    else if (loopCount % 4 == 2)
        myDevice.bitmap( startX + 119, startY + 52, (uint8_t *)Logging_2, Logging_Width, Logging_Height );
    else // if (loopCount % 4 == 3)
        myDevice.bitmap( startX + 119, startY + 52, (uint8_t *)Logging_3, Logging_Width, Logging_Height );

    bool doPartial = ((loopCount % fullUpdateEvery) != 0);

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

    loopCount++;

    while (millis() % 1000 != 0)
        ; // Wait for the next second to elapse
}
