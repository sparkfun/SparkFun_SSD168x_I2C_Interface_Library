// SparkFun_SSD168x_I2C_Interface_Library.h
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

// Header for I2C driver object

#pragma once

// Simple object to encapsulate basic I2C operations.
//
// This is following a pattern for future implementations
//
// This class is focused on Aurduino..

#include <Arduino.h>
#include <Wire.h>

class QwI2C {

public:
    QwI2C(void);

    bool init(void);
    bool init(TwoWire& wirePort);

    // see if a device exists
    bool ping(uint8_t address);

    bool writeRegisterByte(uint8_t address, uint8_t offset, uint8_t data);

    // Write a block of bytes to the device --
    int writeRegisterRegion(uint8_t address, uint8_t offset, uint8_t* data, uint16_t length, unsigned long chunkDelay_ms = 0);

private:
    TwoWire* m_i2cPort;
};