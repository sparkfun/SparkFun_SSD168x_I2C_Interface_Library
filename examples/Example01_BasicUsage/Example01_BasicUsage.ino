#include "SparkFun_SSD168x_I2C_Interface_Library.h"

SSD168xI2CCustom myDevice;

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

    // Do a simple test - fill a rectangle on the screen and then print hello!

    // Fill a rectangle on the screen that has a 4 pixel board
    myDevice.rectangleFill(4, 4, myDevice.getWidth() - 8, myDevice.getHeight() - 8);

    String hello = "hello"; // our message

    // Center our message on the screen. Get the screen size of the "hello" string,
    // calling the getStringWidth() and getStringHeight() methods on the oled

    // starting x position - screen width minus string width  / 2
    int x0 = (myDevice.getWidth() - myDevice.getStringWidth(hello)) / 2;

    // starting y position - screen height minus string height / 2 
    int y0 = (myDevice.getHeight() - myDevice.getStringHeight(hello)) / 2;

    // Draw the text - color of black (0)
    myDevice.text(x0, y0, hello, COLOR_WHITE);


    myDevice.rectangleFill(10,20,30,40,COLOR_WHITE);

    // There's nothing on the screen yet - Now send the graphics to the device
    myDevice.display();
}

void loop()
{
}