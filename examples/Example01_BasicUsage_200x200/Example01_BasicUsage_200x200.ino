#include "SparkFun_SSD168x_I2C_Interface_Library.h"

SSD1681I2C200x200 myDevice;

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