// Example10 : 184x88 testing partial updates - text erase and display bitmap
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

SSD1680I2C184x88Rotated *theDisplay;

// Fonts
#include <res/qw_ep_fnt_10x20.h> // 10x20

// BMP
#include <res/qw_ep_bmp_sparkfun.h>

const unsigned long deepSleep_ms = 100;

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
    theDisplay = new SSD1680I2C184x88Rotated();
    if (theDisplay->begin() == false)
    {
        Serial.println("Device begin failed. Freezing...");
        while (true)
            ;
    }
    Serial.println("Begin success");

}

void loop()
{
    static uint8_t y = 0;

    // ------------------------------------------------

    if (y == 0)
    {
        // Fill the whole screen
        theDisplay->rectangleFill(0, 0, theDisplay->getWidth(), theDisplay->getHeight(), COLOR_OFF);

        // Send the graphics to the device and also set the background for partial updates
        theDisplay->display();

        // Wait for display to update
        do {
            delay(10); // Don't pound the I2C bus too hard
        } while (theDisplay->isBusy());

        theDisplay->deepSleep();

        delay(deepSleep_ms);
    }

    // ------------------------------------------------

    // erase existing text
    theDisplay->erase();

    const char hello[] = "Hello";
    printTextCenter(hello, y++, QW_EP_FONT_10X20, 1, false);

    // Partial update
    theDisplay->display(true);

    // Wait for display to update
    do {
        delay(10); // Don't pound the I2C bus too hard
    } while (theDisplay->isBusy());

    theDisplay->deepSleep();

    delay(deepSleep_ms);

    // ------------------------------------------------

    // erase existing text
    theDisplay->erase();

    const char world[] = "World";
    printTextCenter(world, y++, QW_EP_FONT_10X20, 2, true);

    // Partial update
    theDisplay->display(true);

    // Wait for display to update
    do {
        delay(10); // Don't pound the I2C bus too hard
    } while (theDisplay->isBusy());

    theDisplay->deepSleep();

    delay(deepSleep_ms);

    // ------------------------------------------------

    // erase existing text
    theDisplay->erase();

    theDisplay->bitmap((theDisplay->getWidth() / 2) - (BMP_SPARKFUN_WIDTH / 2), y++, QW_EP_BMP_SPARKFUN);

    // Partial update
    theDisplay->display(true);

    // Wait for display to update
    do {
        delay(10); // Don't pound the I2C bus too hard
    } while (theDisplay->isBusy());

    theDisplay->deepSleep();

    delay(deepSleep_ms);

    // ------------------------------------------------

    if (y >= theDisplay->getHeight())
        y = 0;
}

// Given text, and location, print text center of the screen.
void printTextCenter(const char *text, uint8_t yPos, QwiicEpFont &fontEpType,
                     uint8_t kerning, bool highlight) // text, y, font type, kearning, inverted
{
    theDisplay->setFont(fontEpType);
    theDisplay->setDrawMode(grEpROPXOR);

    uint8_t boxHeight = fontEpType.height;

    uint8_t fontWidth = fontEpType.width;

    uint8_t textPixelWidth = strlen(text) * (fontWidth + kerning);

    // E.g.:
    // 8 chars in the 8X16 font, with kerning 1
    // ((strlen(text) * (fontWidth + kerning)) / 2) = 32
    // (theDisplay->getWidth() / 2) = 32
    // xStart = 0
    // But that looks rubbish if highlight is true
    int xStart = ((int)(theDisplay->getWidth() / 2)) - ((int)(textPixelWidth / 2));
    if (xStart < 0)
        xStart = 0;

    // So, add a gap of 1 pixel if highlight is true and xStart is zero
    if (highlight && (xStart == 0))
        xStart = 1;

    uint8_t xPos = xStart;
    for (int x = 0; x < strlen(text); x++)
    {
        theDisplay->setCursor(xPos, yPos);
        theDisplay->print(text[x]);
        xPos += fontWidth + kerning;
    }

    if (highlight) // Draw a box, inverted over text
    {
        // Error check
        int xBoxStart = xStart - 5;
        int xBoxWidth = textPixelWidth + 9;
        if (xBoxStart < 0)
        {
            xBoxWidth += xBoxStart * 2; // Shrink the width by twice the excess
            xBoxStart = 0;
        }
        if ((xBoxStart + xBoxWidth) > theDisplay->getWidth())
            xBoxWidth = theDisplay->getWidth() - xBoxStart;

        theDisplay->rectangleFill(xBoxStart, yPos, xBoxWidth, boxHeight, 1); // x, y, width, height, color
    }
}

