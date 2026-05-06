// qwiic_ssd1681_200x200.h
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

// Implementation for a generic 200x200 SSD1681 device

#pragma once

#include "i2c_ssd1681.h"

//////////////////////////////////////////////////////////////////
// Set the defaults for a generic 200x200 SSD1681 display

#define kI2cSsd1681200x200Width 200
#define kI2cSsd1681200x200Height 200

#define kI2cSsd1681200x200XOffset 0
#define kI2cSsd1681200x200YOffset 0

#define kI2cSsd1681200x200DefaultAddress 0x48

class I2cSsd1681_200x200 : public I2cSsd1681 {

public:
    // Constructor - setup the viewport and default address for this device.
    I2cSsd1681_200x200()
        : I2cSsd1681(kI2cSsd1681200x200XOffset, kI2cSsd1681200x200YOffset, kI2cSsd1681200x200Width, kI2cSsd1681200x200Height)
    {
        default_address = kI2cSsd1681200x200DefaultAddress;
    };

    ~I2cSsd1681_200x200()
    {
        if (m_graphicsBuffer != nullptr)
        {
            delete[] m_graphicsBuffer;
            m_graphicsBuffer = nullptr;
        }
    };

    // set up the specific device settings
    bool init(void)
    {
        if (m_graphicsBuffer != nullptr)
            delete[] m_graphicsBuffer;
        m_graphicsBuffer = new uint8_t[(uint16_t)kI2cSsd1681200x200Width * (uint16_t)kI2cSsd1681200x200Height / 8];
        this->I2cSsd1681::setBuffer(m_graphicsBuffer); // The buffer to use

        // Call the super class to do all the work
        return this->I2cSsd1681::init();
    };

private:
    // Graphics buffer for this device.
    uint8_t *m_graphicsBuffer = nullptr;
};