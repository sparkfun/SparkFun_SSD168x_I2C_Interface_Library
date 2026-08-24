// i2c_ssd1681_defs.h
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
 * Header file for the SSD1681 bitmap graphics driver device.
*/

#pragma once

#include "Arduino.h"

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
#define kCmdSsd1681WriteRamBW 0x24
#define kCmdSsd1681WriteRamRed 0x26
#define kCmdSsd1681ReadRam 0x27
#define kCmdSsd1681WriteVcom 0x2C
#define kCmdSsd1681ReadOtp 0x2D
#define kCmdSsd1681ReadUserID 0x2E
#define kCmdSsd1681ReadStatus 0x2F
#define kCmdSsd1681WriteLut 0x32
#define kCmdSsd1681DisplayOption 0x37
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
  const uint8_t followingBytes[3];
  const bool delayAfter;
  const unsigned long delayDuration;
  const bool checkBusyAfter;
} ssd1681InitCodeEntry;

const ssd1681InitCodeEntry ssd1681InitCode[] = {
  { kCmdSsd1681SwReset, 0, { 0 }, true, 20, true },
  { kCmdSsd1681TempSensorControl, 1, { 0x80 }, false, 0, false }, // Internal temperature sensor
  { kCmdSsd1681WriteBorder, 1, { 0x05 }, false, 0, false }, // Follow LUT1
  { kCmdSsd1681SetRamCounterX, 1, { 0 }, false, 0, false },
  { kCmdSsd1681SetRamCounterY, 2, { 0, 0 }, false, 0, false },
  { kCmdSsd1681DisplayUpdateCtrl1, 1, { 0x88 }, false, 0, false }, // **Inverse** Red RAM, **Inverse** BW RAM content
};

const int numSsd1681InitCodeEntries = sizeof(ssd1681InitCode) / sizeof(ssd1681InitCodeEntry);

#define kMaxPageNumberSSD1681 25 // 200 / 8
