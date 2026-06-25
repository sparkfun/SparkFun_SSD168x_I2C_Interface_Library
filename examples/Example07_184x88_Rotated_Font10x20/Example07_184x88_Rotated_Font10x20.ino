// Example07 : Font 10x20 (184x88 rotated)
//
// Written by P.C. @ SparkFun Electronics, June 2026
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
#include <res/qw_ep_fnt_10x20.h>

void setup()
{
    delay(1000);
    
    // Start serial
    Serial.begin(115200);
    Serial.println("Running OLED example");

    Wire.begin();

    // Wake and initalize the device and related graphics system
    if (myDevice.begin() == false)
    {
        Serial.println("Device begin failed. Freezing...");
        while (true)
            ;
    }
    Serial.println("Begin success");

    // Display our text
    myDevice.setFont(QW_EP_FONT_10X20);
    myDevice.text(0,  0, "0123456789ABCDEFGH");
    myDevice.text(0, 20, "IJKLMNOPQRSTUVWXYZ");
    myDevice.text(0, 40, "!#$%&()*+-abcdefgh");
    myDevice.text(0, 60, "ijklmnopqrstuvwxyz");

    // There's nothing on the screen yet - Now send the graphics to the device
    unsigned long startTime = millis();

    myDevice.display();

    Serial.printf("myDevice.display() took %ldms\r\n", millis() - startTime);

    // Wait for the display to update
    startTime = millis();

    while (myDevice.isBusy())
        delay(10); // Don't pound the I2C bus too hard
    
    Serial.printf("myDevice was busy for %ldms\r\n", millis() - startTime);

    // Now put the display to sleep
    myDevice.deepSleep();
}

void loop()
{
}