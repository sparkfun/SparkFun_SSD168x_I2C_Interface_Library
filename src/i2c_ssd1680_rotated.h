// i2c_ssd1680_rotated.h
//
// Written by P.C. @ SparkFun Electronics, April 2026
//
// This is a library to control SSD1680/1 e-Paper displays via I2C, using a I2C to SPI Bridge.
//
// SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT
//
//    The MIT License (MIT)
//
//    Copyright (c) 2026 SparkFun Electronics
//    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
//    associated documentation files (the "Software"), to deal in the Software without restriction,
//    including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
//    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
//    do so, subject to the following conditions:
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*
 * Header file for the SSD1680 bitmap graphics driver device.
*/

#pragma once

#include "i2c_ssd1680_defs.h"
#include "qwiic_grbuffer.h"
#include "qwiic_i2c.h"
#include "res/qwiic_resdef.h"
#include "qwiic_grcommon.h"

/////////////////////////////////////////////////////////////////////////////
// Buffer Management
/////////////////////////////////////////////////////////////////////////////
//
// SSD1680 supports up to 296 rows and 176 columns
// Here, everything is rotated, becoming similar to the SSD1306
// Each memory byte contains the pixels for 8 'rows'
// The MSB is the top-most pixel
// By default: white pixels are '1', black are '0', but UpdateCtrl1 can invert this
// Because this library is hard-wired to expect 'off' pixels to be '0' and 'on' pixels to be '1',
// it is MUCH easier if we use the inversion...
// To make life easier for ourselves, we use:
//   Y decrement, X increment mode
//   with the address counter updating in the Y direction
// This places 'Page' 0 at the top of the page
// In our memory buffer, we can use standard X,Y coordinates
// When we write to the display, X becomes Y
// We just need to remember to set the Y RAM Counter to (e.g.) 183 initially
//
// The 'pages' are arranged like this:
//              This corner is 0,0 in display coordinates
//   __________/
// _|__________| < Page 0 ('Rows' 0-7)
// F|__________| < Page 1 ('Rows' 8-15)
// P|__________| < Page 2 ('Rows' 16-23)
// C|__________|
// -|__________|
//  |__________|
//            ^- 'Column' 0
//   ^- 'Column' 295
//
// The memory/back buffer of the SSD1680 is based on the concept of pages -
// each page is a stream of bytes, and - with addresses increasing in Y - is defined as follows:
//
//      - Y pixel position is a bit in a byte, so a page can have 8 Y locations
//      - X pixel position is an offset in a byte array. The first byte is right-most
//
// With inversion, a pixel value of 0 is 'off' (white), 1 id 'on' (black)
//
// This implementation uses the Page mode for buffer transfer. This is defined by:
//     - A start position is set - a page number and 'column' in that page.
//     - As data is transferred, it is written to the screenbuffer, based on this start
//       position
//     - If the end of the page is reached, the next entry location is the start of the next page
//
// ** Note: **
//    Because we are rotated, with the FPC to the left, X coordinates need to be flipped:
//    If the X coordinate is 0, the pixel is in 'column' 183 (on 184x88 displays)
//
// >> Implementation <<
//
// This implementation uses the concept of "dirty rects" at the page level to minimize data
// transfers to the device. The min and max Y locations set for each page is recorded as
// graphics are drawn to the graphics buffer. When the transfering the display buffer to
// the devices screen buffer, the following takes place:
//
//      For each page:
//          - if page is dirty
//              - Set the screen buffer current location to this page, min dirty value
//              - Write buffer bytes to the device - starting at min for the page, ending at max
//              - Mark the buffer as "clean"
//
//

/////////////////////////////////////////////////////////////////////////////
// I2cSsd1680Rotated
// A buffer graphics device to support the SSD1680 graphics hardware

class I2cSsd1680Rotated : public QwGrBufferDevice
{
  private:
    void setupDefaults(void);

  public:
    I2cSsd1680Rotated()
    {
        setupDefaults(); // default constructor - always called
    }
    I2cSsd1680Rotated(uint8_t width, uint8_t height) : I2cSsd1680Rotated(0, 0, width, height){};

    // call super class
    I2cSsd1680Rotated(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height) : QwGrBufferDevice(x0, y0, width, height)
    {
        setupDefaults();
    };

    // Public draw methods
    void display(bool partial = false, bool background = false); // send graphics buffer to the device screen buffer
    void erase(void);

    // Device setup
    virtual bool init(void);

    bool isInitialized(void)
    {
        return m_isInitialized;
    };
    bool reset(bool clearDisplay = true);

    // method to set the communication bus this object should use
    void setCommBus(QwI2C &theBus, uint8_t id_bus);

    // Set the current color/pixel write operation
    void setColor(uint8_t color);

    // default address of the device - expect the sub to fill in.
    uint8_t default_address;

    void setRasterOp(grRasterOp_t rop)
    {
        m_rop = rop;
    }

    grRasterOp_t rasterOp(void)
    {
        return m_rop;
    }

    void deepSleep(bool mode2); // Only a hardware reset can wake it again

    bool isBusy(void); // Return the state of the BUSY pin

  protected:
    // Subclasses of this class define the specifics of the device, including size.
    // Subclass needs to define the graphics buffer array - stack based - and pass in
    void setBuffer(uint8_t *pBuffer);

    ///////////////////////////////////////////////////////////////////////////
    // Internal, fast draw routines - this are used in the overall
    // draw interface (_QwIDraw) for this object/device/system.
    //
    // >> Pixels <<
    void drawPixel(uint8_t x, uint8_t y, uint8_t clr);

    // >> Fast Lines <<
    void drawLineHorz(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr);
    void drawLineVert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr);

    // fast rect fill
    void drawRectFilled(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height, uint8_t clr);

    // >> Fast Bitmap <<
    void drawBitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *pBitmap, uint8_t bmp_width,
                    uint8_t bmp_height);

    ///////////////////////////////////////////////////////////////////////////
    // configuration methods for sub-classes. Settings unique to a device

  private:
    // Internal buffer management methods
    bool setScreenBufferAddress(uint8_t page, uint8_t columnStart, uint8_t columnEnd);
    void initBuffers(void); // clear graphics and screen buffer
    void clearScreenBuffer(void);
    void resendGraphics(void);
    void setupEpaperDevice(bool clearBuffer = true);

    // device communication methods
    void sendDevCommand(uint8_t command);
    void sendDevCommand(uint8_t command, uint8_t value);
    void sendDevCommand(uint8_t command, uint8_t *values, uint8_t n_values);
    void sendDevData(uint8_t *pData, uint8_t nData);
    void sendDevReset(void);
    uint8_t readDevStatus(void);

    /////////////////////////////////////////////////////////////////////////////
    // instance vars

    // Buffer variables
    uint8_t *m_pBuffer;                      // Pointer to the graphics buffer
    uint8_t m_nPages;                        // number of pages for current device
    pageState_t m_pageState[kMaxPageNumberSSD1680]; // page state descriptors
    pageState_t m_pageErase[kMaxPageNumberSSD1680]; // keep track of erase boundaries
    bool m_pendingErase;

    // display variables
    uint8_t m_color;    // current color (really 0 or 1)
    grRasterOp_t m_rop; // current raster operation code

    // I2C  things
    QwI2C *m_i2cBus;      // pointer to our i2c bus object
    uint8_t m_i2cAddress; // address of the device

    // Stash values for settings that are unique to each device.

    bool m_isInitialized; // general init flag

    // Handy helper
    const uint8_t gfx_byte_bits[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
};
