#include "SparkFun_SSD168x_I2C_Interface_Library.h"

SSD1680I2C184x88Rotated myDevice;

// Fonts
#include <res/qw_fnt_largenum.h>

// Bitmap
#include "res/qw_bmp_sparkfun.h"

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

    // Do a simple test - fill a rectangle on the screen and then print text and graphic

    // Fill a rectangle on the screen that has a 4 pixel border
    myDevice.rectangleFill(4, 4, myDevice.getWidth() - 8, myDevice.getHeight() - 8);

    // Fill a rectangle within that, to leave an 4 pixel frame
    myDevice.rectangleFill(8, 8, myDevice.getWidth() - 16, myDevice.getHeight() - 16, COLOR_OFF);

    // Add a logo
    myDevice.bitmap(8, 20, QW_BMP_SPARKFUN);

    // Display our text
    myDevice.setFont(QW_FONT_LARGENUM);
    myDevice.text(76, 20, "01234567");

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