// i2c_ssd1680_defs.h
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
 * Header file for the SSD1680 bitmap graphics driver device.
*/

#pragma once

#include "Arduino.h"

/////////////////////////////////////////////////////////////////////////////
// Device Commands
//
// The commands are codes used to communicate with the SSD1680 device and are
// from the devices datasheet.
//

#define kCmdSsd1680DriverOutput 0x01
#define kCmdSsd1680GateDrivingVoltage 0x03
#define kCmdSsd1680SourceDrivingVoltage 0x04
#define kCmdSsd1680ProgramInitialSetting 0x08
#define kCmdSsd1680ProgramRegisterWrite 0x09
#define kCmdSsd1680ProgramRegisterRead 0x0A
#define kCmdSsd1680BoostSoftStart 0x0C
#define kCmdSsd1680DeepSleep 0x10
#define kCmdSsd1680DataEntryMode 0x11
#define kCmdSsd1680SwReset 0x12
#define kCmdSsd1680HVReadyDetect 0x14
#define kCmdSsd1680VCIDetect 0x15
#define kCmdSsd1680TempSensorControl 0x18
#define kCmdSsd1680TempSensorWrite 0x1A
#define kCmdSsd1680TempSensorRead 0x1B
#define kCmdSsd1680MasterActivate 0x20
#define kCmdSsd1680DisplayUpdateCtrl1 0x21
#define kCmdSsd1680DisplayUpdateCtrl2 0x22
#define kCmdSsd1680WriteRamBW 0x24
#define kCmdSsd1680WriteRamRed 0x26
#define kCmdSsd1680ReadRam 0x27
#define kCmdSsd1680WriteVcom 0x2C
#define kCmdSsd1680ReadOtp 0x2D
#define kCmdSsd1680ReadUserID 0x2E
#define kCmdSsd1680ReadStatus 0x2F
#define kCmdSsd1680WriteLut 0x32
#define kCmdSsd1680WriteBorder 0x3C
#define kCmdSsd1680SetRamPosX 0x44
#define kCmdSsd1680SetRamPosY 0x45
#define kCmdSsd1680AutoWriteRed 0x46
#define kCmdSsd1680AutoWriteBW 0x47
#define kCmdSsd1680SetRamCounterX 0x4E
#define kCmdSsd1680SetRamCounterY 0x4F
#define kCmdSsd1680NOP 0x7F

/////////////////////////////////////////////////////////////////////////////
// Device Config
/////////////////////////////////////////////////////////////////////////////
//
// Defaults
// Each device can have a different Hardware pin configuration, which must
// be set in the device. These are the pins that connect the display to
// the SSD1680.
//

typedef struct {
  const uint8_t command;
  const uint8_t numFollowingBytes;
  const uint8_t followingBytes[3];
  const bool delayAfter;
  const unsigned long delayDuration;
  const bool checkBusyAfter;
} ssd1680InitCodeEntry;

const ssd1680InitCodeEntry ssd1680InitCode[] = {
  { kCmdSsd1680SwReset, 0, { 0 }, true, 20, true },
  { kCmdSsd1680TempSensorControl, 1, { 0x80 }, false, 0, false }, // Internal temperature sensor
  //{ kCmdSsd1680DisplayUpdateCtrl2, 1, { 0xB1 }, false, 0, false }, // Load temperature value
  //{ kCmdSsd1680MasterActivate, 0, { 0 }, true, 10, true }, // Display update sequence
  //{ kCmdSsd1680TempSensorWrite, 2, { 0x5A, 0 }, false, 0, false }, // 4-Gray
  //{ kCmdSsd1680DisplayUpdateCtrl2, 1, { 0x91 }, false, 0, false }, // Load temperature value
  //{ kCmdSsd1680MasterActivate, 0, { 0 }, true, 10, true }, // Display update sequence
  { kCmdSsd1680WriteBorder, 1, { 0x05 }, false, 0, false }, // Follow LUT1 (White)
  { kCmdSsd1680SetRamCounterX, 1, { 0 }, false, 0, false },
  { kCmdSsd1680SetRamCounterY, 2, { 0, 0 }, false, 0, false },
  { kCmdSsd1680DisplayUpdateCtrl1, 2, { 0x88, 0x80 }, false, 0, false }, // **Inverted** Red RAM, **Inverted** BW RAM content, S8-167
  { kCmdSsd1680WriteVcom, 1, { 0x08 }, false, 0, false }, // VCOM -0.2
  //{ kCmdSsd1680WriteVcom, 1, { 0x36 }, false, 0, false }, // VCOM -1.35
  //{ kCmdSsd1680GateDrivingVoltage, 1, { 0x17 }, false, 0, false }, // VGH 20
  //{ kCmdSsd1680SourceDrivingVoltage, 3, { 0x41, 0xAC, 0x32 }, false, 0, false }, // VSH1 15V, VSH2 5.4V, VSL -15V
};

const int numSsd1680InitCodeEntries = sizeof(ssd1680InitCode) / sizeof(ssd1680InitCodeEntry);

#define kMaxPageNumberSSD1680 22 // 176 / 8
