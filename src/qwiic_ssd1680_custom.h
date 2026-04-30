// qwiic_ssd1680_custom.h
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

// Implementation for a generic 200x200 SSD1681 device

#pragma once

#include "i2c_ssd1680.h"

//////////////////////////////////////////////////////////////////
// Set the defaults for a generic 184x88 SSD1680 display

#define kI2cSsd1681184x88DefaultWidth 88
#define kI2cSsd1681184x88DefaultHeight 184

#define kI2cSsd1681184x88DefaultXOffset 0
#define kI2cSsd1681184x88DefaultYOffset 0

#define kI2cSsd1681184x88DefaultAddress 0x48

class I2cSsd1680_Custom : public I2cSsd1680 {

public:
    // Constructor - setup the viewport and default address for this device.
    I2cSsd1680_Custom()
        : I2cSsd1680(kI2cSsd1681184x88DefaultXOffset, kI2cSsd1681184x88DefaultYOffset, kI2cSsd1681184x88DefaultWidth, kI2cSsd1681184x88DefaultHeight)
    {
        default_address = kI2cSsd1681184x88DefaultAddress;
    };

    ~I2cSsd1680_Custom()
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
        this->I2cSsd1680::setViewport(m_xOffset, m_yOffset, m_displayWidth, m_displayHeight);

        if (m_graphicsBuffer != nullptr)
            delete[] m_graphicsBuffer;
        m_graphicsBuffer = new uint8_t[(uint16_t)m_displayWidth * (uint16_t)m_displayHeight / 8];
        this->I2cSsd1680::setBuffer(m_graphicsBuffer); // The buffer to use

        // Call the super class to do all the work
        return this->I2cSsd1680::init();
    };

    void setXOffset(uint8_t xOffset){ m_xOffset = xOffset; }
    void setYOffset(uint8_t yOffset){ m_yOffset = yOffset; }
    void setDisplayWidth(uint8_t displayWidth){ m_displayWidth = displayWidth; }
    void setDisplayHeight(uint8_t displayHeight){ m_displayHeight = displayHeight; }

private:
    uint8_t m_xOffset = kI2cSsd1681184x88DefaultXOffset;
    uint8_t m_yOffset = kI2cSsd1681184x88DefaultYOffset;
    uint8_t m_displayWidth = kI2cSsd1681184x88DefaultWidth;
    uint8_t m_displayHeight = kI2cSsd1681184x88DefaultHeight;

    // Graphics buffer for this device.
    uint8_t *m_graphicsBuffer = nullptr;
};