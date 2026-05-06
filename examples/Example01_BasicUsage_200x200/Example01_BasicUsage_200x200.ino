// Example01 : Basic usage (200x200)
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

SSD1681I2C200x200 myDevice;

// Fonts
#include <res/qw_fnt_largenum.h>

// Bitmap
#include <res/qw_bmp_sparkfun.h>

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

    // Do a simple test - fill a rectangle on the screen and then print text and graphic

    // Fill a rectangle on the screen that has a 8 pixel border
    myDevice.rectangleFill(8, 8, myDevice.getWidth() - 16, myDevice.getHeight() - 16);

    // Fill a rectangle within that, to leave an 8 pixel frame
    myDevice.rectangleFill(16, 16, myDevice.getWidth() - 32, myDevice.getHeight() - 32, COLOR_OFF);

    // Add a logo
    myDevice.bitmap(16, 75, QW_BMP_SPARKFUN);

    // Display our text
    myDevice.setFont(QW_FONT_LARGENUM);
    myDevice.text(80, 75, "01234567");

    // There's nothing on the screen yet - Now send the graphics to the device
    myDevice.display();

    // Wait for display to update
    while (myDevice.isBusy())
        delay(10); // Don't pound the I2C bus too hard
    
    // Now put the display to sleep
    myDevice.deepSleep();
}

void loop()
{
}