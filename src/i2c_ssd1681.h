// i2c_ssd1681.h
//
// Written by P.C. @ SparkFun Electronics, April 2026
//
// This is an experimental library to control SSD1680/1 e-Paper displays via I2C, using a TI MSP430FR2433 as the I2C to SPI Bridge.
//
// The MSP430FR2433 is configured as a I2C peripheral with two registers: Control (Register 0x00), and Data (Register 0x01).
// All data written to Register 0x00 is bridged to SPI with the D/C# pin held low.
// All data written to Register 0x01 is bridged to SPI with the D/C# pin held high.
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
 * Header file for the SSD1681 bitmap graphics driver device.
 */

#pragma once

#include "qwiic_grbuffer.h"
#include "qwiic_i2c.h"
#include "res/qwiic_resdef.h"
#include "qwiic_grcommon.h"

/////////////////////////////////////////////////////////////////////////////
// Device Commands
//
// The commands are codes used to communicate with the SSD1681 device and are
// from the devices datasheet.
//

#define kCmdSsd1681DriverOutput 0x01
#define kCmdSsd1681GateDrivingVoltage 0x03
#define kCmdSsd1681SourceDrivingVoltage 0x04
#define kCmdSsd1681ProgramInitialSetting 0x08
#define kCmdSsd1681ProgramRegisterWrite 0x09
#define kCmdSsd1681ProgramRegisterRead 0x0A
#define kCmdSsd1681BoostSoftStart 0x0C
#define kCmdSsd1681DeepSleep 0x10
#define kCmdSsd1681DataEntryMode 0x11
#define kCmdSsd1681SwReset 0x12
#define kCmdSsd1681HVReadyDetect 0x14
#define kCmdSsd1681VCIDetect 0x15
#define kCmdSsd1681TempSensorControl 0x18
#define kCmdSsd1681TempSensorWrite 0x1A
#define kCmdSsd1681TempSensorRead 0x1B
#define kCmdSsd1681MasterActivate 0x20
#define kCmdSsd1681DisplayUpdateCtrl1 0x21
#define kCmdSsd1681DisplayUpdateCtrl2 0x22
#define kCmdSsd1681WriteRam1 0x24
#define kCmdSsd1681WriteRam2 0x26
#define kCmdSsd1681ReadRam 0x27
#define kCmdSsd1681WriteVcom 0x2C
#define kCmdSsd1681ReadOtp 0x2D
#define kCmdSsd1681ReadUserID 0x2E
#define kCmdSsd1681ReadStatus 0x2F
#define kCmdSsd1681WriteLut 0x32
#define kCmdSsd1681WriteBorder 0x3C
#define kCmdSsd1681SetRamPosX 0x44
#define kCmdSsd1681SetRamPosY 0x45
#define kCmdSsd1681AutoWriteRed 0x46
#define kCmdSsd1681AutoWriteBW 0x47
#define kCmdSsd1681SetRamCounterX 0x4E
#define kCmdSsd1681SetRamCounterY 0x4F
#define kCmdSsd1681NOP 0x7F

/////////////////////////////////////////////////////////////////////////////
// Device Config
/////////////////////////////////////////////////////////////////////////////
//
// Defaults
// Each device can have a different Hardware pin configuration, which must
// be set in the device. These are the pins that connect the display to
// the SSD1681.
//

typedef struct {
  const uint8_t command;
  const uint8_t numFollowingBytes;
  const uint8_t followingBytes[2];
  const bool delayAfter;
  const unsigned long delayDuration;
} ssd1681InitCodeEntry;

const ssd1681InitCodeEntry ssd1681InitCode[] = {
  { kCmdSsd1681SwReset, 0, { 0 }, true, 20 },
  { kCmdSsd1681DataEntryMode, 1, { 0x07 }, false, 0 }, // Update in Y direction, Y increment, X increment
  { kCmdSsd1681WriteBorder, 1, { 0x05 }, false, 0 }, // Follow LUT1
  { kCmdSsd1681TempSensorControl, 1, { 0x80 }, false, 0 }, // Internal temperature sensor
  { kCmdSsd1681SetRamCounterX, 1, { 0 }, false, 0 },
  { kCmdSsd1681SetRamCounterY, 2, { 0, 0 }, false, 0 },
};

const int numSsd1681InitCodeEntries = sizeof(ssd1681InitCode) / sizeof(ssd1681InitCodeEntry);

/////////////////////////////////////////////////////////////////////////////
// Buffer Management
/////////////////////////////////////////////////////////////////////////////
//
// The memory/back buffer of the SSD1681 is based on the concept of pages -
// each page is a stream of bytes, and defined as follows:
//
//      - X pixel position is an offset in a byte array
//      - Y pixel position is a bit in a byte, so a page can have 8 Y locations
//
// A pixel value of 1, turn on the corresponding pixel, 0 turns it off.
//
// The device has different data transfer modes - see the data sheet - mostly
// outline how received a recieved byte is placed in the device framebuffer and the
// next update locaton set.
//
// This implementation uses the Page mode for buffer transfer. This is defined by:
//     - A start position is set - a page number and column in that page.
//     - As data is transferred, it is written to the screenbuffer, based on this start
//       position
//     - If the end of the page is reached, the next entry location is the start of that page
//
// >> Implementation <<
//
// This implementation uses the concept of "dirty rects" at the page level to minimize data
// transfers to the device. The min and max x locations set for each page is recorded as
// graphics are draw to the graphics buffer. When the transfering the display buffer to
// the devices screen buffer, the following takes place:
//
//      For each page:
//          - if page is dirty
//              - Set the screen buffer current location to this page, xmin dirty value
//              - Write buffer bytes to the device - starting at xmin for the page, ending at xmax
//              - Mark the buffer as "clean"
//
//

#define kMaxPageNumberSSD1681 25 // 200 / 8

/////////////////////////////////////////////////////////////////////////////
// I2cSsd1681
// A buffer graphics device to support the SSD1681 graphics hardware

class I2cSsd1681 : public QwGrBufferDevice
{
  private:
    void setupDefaults(void);

  public:
    I2cSsd1681()
    {
        setupDefaults(); // default constructor - always called
    }
    I2cSsd1681(uint8_t width, uint8_t height) : I2cSsd1681(0, 0, width, height){};

    // call super class
    I2cSsd1681(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height) : QwGrBufferDevice(x0, y0, width, height)
    {
        setupDefaults();
    };

    // Public draw methods
    void display(bool partial = false); // send graphics buffer to the devices screen buffer
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

    // Settings/operational methods
    void setContrast(uint8_t);

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

    void displayPower(bool enable = true);

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
    bool setScreenBufferAddress(uint8_t page, uint8_t column);
    void initBuffers(void); // clear graphics and screen buffer
    void clearScreenBuffer(void);
    void resendGraphics(void);
    void setupEpaperDevice(bool clearDisplay = true);

    // device communication methods
    void sendDevCommand(uint8_t command);
    void sendDevCommand(uint8_t command, uint8_t value);
    void sendDevCommand(uint8_t command, uint8_t *values, uint8_t n_values);
    void sendDevData(uint8_t *pData, uint8_t nData);

    /////////////////////////////////////////////////////////////////////////////
    // instance vars

    // Buffer variables
    uint8_t *m_pBuffer;                      // Pointer to the graphics buffer
    uint8_t m_nPages;                        // number of pages for current device
    pageState_t m_pageState[kMaxPageNumberSSD1681]; // page state descriptors
    pageState_t m_pageErase[kMaxPageNumberSSD1681]; // keep track of erase boundaries
    bool m_pendingErase;

    // display variables
    uint8_t m_color;    // current color (really 0 or 1)
    grRasterOp_t m_rop; // current raster operation code

    // I2C  things
    QwI2C *m_i2cBus;      // pointer to our i2c bus object
    uint8_t m_i2cAddress; // address of the device

    // Stash values for settings that are unique to each device.

    bool m_isInitialized; // general init flag
};
