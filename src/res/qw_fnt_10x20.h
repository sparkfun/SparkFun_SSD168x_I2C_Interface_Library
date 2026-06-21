// qw_fnt_10x20.h
//
// Written by P.C. @ SparkFun Electronics, June 2026
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

#pragma once

#include "qwiic_resdef.h"

class QwEpFont10x20 final : public fontEpSingleton<QwEpFont10x20> {

public:
    const uint8_t* data(void)
    {
        // include font data (static const), and attribute defines.
        // Doing this here makes the data variable a static (aka only one instance ever)
        // variable in this method.
#include "_fnt_10x20.h"

        return font10x20_data;
    }

    QwEpFont10x20()
        : fontEpSingleton<QwEpFont10x20>(FONT_10x20_WIDTH,
            FONT_10x20_HEIGHT,
            FONT_10x20_START,
            FONT_10x20_NCHAR,
            FONT_10x20_MAP_WIDTH,
            FONT_10x20_NAME)
    {
    }
};

#define QW_EP_FONT_10x20 QwEpFont10x20::instance()