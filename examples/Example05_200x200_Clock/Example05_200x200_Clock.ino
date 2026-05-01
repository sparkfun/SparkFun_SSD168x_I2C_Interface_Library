#include "SparkFun_SSD168x_I2C_Interface_Library.h"

SSD1681I2C200x200 myDevice;

// Fonts
#include <res/qw_fnt_largenum.h>

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

    // Fill a rectangle on the screen that has a 4 pixel border
    myDevice.rectangleFill(4, 4, myDevice.getWidth() - 8, myDevice.getHeight() - 8);

    // Fill a rectangle within that, to leave an 4 pixel frame
    myDevice.rectangleFill(8, 8, myDevice.getWidth() - 16, myDevice.getHeight() - 16, COLOR_OFF);

    // There's nothing on the screen yet - Now send the graphics to the device
    myDevice.display();

    // Wait for display to update
    while (myDevice.isBusy())
        delay(10); // Don't pound the I2C bus too hard
    
    myDevice.setFont(QW_FONT_LARGENUM);
}

void loop()
{
    // char arrays to hold the time and previous time
    char theTime[strlen("HHHH:MM:SS") + 1];
    static char previousTime[strlen("HHHH:MM:SS") + 1] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };

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
        // wake (reset) the display
        myDevice.reset(false);

        // for each character that has changed, erase it and update it
        for (int i = 0; i < strlen("HHHH:MM:SS"); i++)
        {
            const int xStart = 40;
            const int yStart = 76;
            if (charsChanged[i])
            {
                myDevice.rectangleFill(xStart + i * FONT_LARGENUM_WIDTH, yStart,
                                FONT_LARGENUM_WIDTH, FONT_LARGENUM_HEIGHT, COLOR_OFF);
                char newChar[2];
                sprintf(newChar, "%c", theTime[i]);
                myDevice.text(xStart + i * FONT_LARGENUM_WIDTH, yStart, newChar);
            }
        }

        // if only 1 character has changed, do a partial update
        if (numCharsChanged <= 1)
            myDevice.display(true);
        // otherside do a full update
        else
            myDevice.display();

        // Wait for display to update
        while (myDevice.isBusy())
            delay(10); // Don't pound the I2C bus too hard

        // Now put the display to sleep
        myDevice.deepSleep();
    }

    // update previousTime
    strcpy(previousTime, theTime);
}