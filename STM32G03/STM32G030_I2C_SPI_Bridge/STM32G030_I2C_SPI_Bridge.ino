/* STM32 (G030) I2C to SPI Bridge - for e-paper displays

By P.C. for SparkFun Electronics, May 6th 2026

Based loosely on example code from the STM32 Boards package

This code runs on an STM32G030 processor. Tested on a generic STM32G030F6P6 dev board (32KB Flash, 20-pin TSSOP)

This code acts as a I2C to SPI bridge, to interface with SSD168x e-paper displays

Set USART Support to Disabled (no Serial support) - we can not access USART1 TX and RX

Connections:
Pin 1   PB7  I2C1 SDA  : Alternate Function 6 (AF6)
Pin 7   PA0  e-paper D/C#
Pin 8   PA1  e-paper BUSY
Pin 9   PA2  e-paper RST
Pin 12  PA5  SPI1 SCK  : Alternate Function 0 (AF0)
Pin 13  PA6  SPI1 MISO : Alternate Function 0 (AF0) : not used
Pin 14  PA7  SPI1 MOSI : Alternate Function 0 (AF0) : data to e-paper
Pin 15  PB0  SPI CS / SS
Pin 20  PB6  I2C1 SCL  : Alternate Function 6 (AF6)

This code emulates a single I2C peripheral (address 0x48, unshifted) with three 'registers':
Control (Register 0x00) : writes to this are bridged to SPI with the D/C# pin held low
Data    (Register 0x01) : writes to this are bridged to SPI with the D/C# pin held high
Reset   (Register 0x02) : writes to this generate a reset pulse on RST

I2C reads from the peripheral return bytes containing the e-paper BUSY flag in the LSB

Written to support the SparkFun SSD168x I2C Interface Library:
https://github.com/sparkfun/SparkFun_SSD168x_I2C_Interface_Library

SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).

SPDX-License-Identifier: MIT

  The MIT License (MIT)

  Copyright (c) 2026 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <Wire.h>
#include <SPI.h>

const uint8_t I2C_ADDR = 0x48;
const uint8_t CONTROL_REG = 0x00;
const uint8_t DATA_REG = 0x01;
const uint8_t RESET_REG = 0x02;
const int DC = PA_0;
const int BUSY = PA_1;
const int RST = PA_2;
const int CS = PB_0;

const SPISettings spiSettings = { 2000000, MSBFIRST, SPI_MODE0 };

static volatile uint8_t counter = 0;
const uint8_t maxCount = 200; // I2C receive is limited to 32 bytes. TODO: can this be increased?
static uint8_t i2cBuffer[maxCount + 1]; // i2cBuffer[0] contains the 'register' address
static uint8_t i2cBufferCopy[maxCount + 1];
static volatile uint8_t bytesToSend;

void setup()
{
  pinMode(DC, OUTPUT);
  pinMode(BUSY, INPUT);
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(PA_5, OUTPUT);
  pinMode(PA_6, INPUT);
  pinMode(PA_7, OUTPUT);
  SPI.setSCLK(PA_5);
  SPI.setMISO(PA_6);
  SPI.setMOSI(PA_7);
  //SPI.setSSEL(PB_9);
  SPI.begin();

  SPI.beginTransaction(spiSettings); // Dummy transaction to set clock speed
  SPI.transfer(0x7F); // SSD168x NOP
  SPI.endTransaction();

  Wire.setSDA(PB_7);
  Wire.setSCL(PB_6);
  Wire.begin(I2C_ADDR);         // join i2c bus with address I2C_ADDR
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // register event
}

void loop()
{
  if (counter > 0)
  {
    bytesToSend = counter;
    memcpy(i2cBufferCopy, i2cBuffer, counter); // Copy the buffer
    counter = 0;
  }

  if (bytesToSend > 0)
  {
    if (i2cBufferCopy[0] == CONTROL_REG)
    {
      if (bytesToSend > 1)
      {
        digitalWrite(DC, LOW);
        digitalWrite(CS, LOW);
        for (uint8_t i = 1; bytesToSend > 1; i++)
        {
          SPI.transfer(i2cBufferCopy[i], SPI_TRANSMITONLY);
          bytesToSend--;
        }
        digitalWrite(CS, HIGH);
        digitalWrite(DC, HIGH);
      }
    }
    else if (i2cBufferCopy[0] == DATA_REG)
    {
      if (bytesToSend > 1)
      {
        digitalWrite(DC, HIGH);
        digitalWrite(CS, LOW);
        for (uint8_t i = 1; bytesToSend > 1; i++)
        {
          SPI.transfer(i2cBufferCopy[i], SPI_TRANSMITONLY);
          bytesToSend--;
        }
        digitalWrite(CS, HIGH);
      }
    }
    else if (i2cBufferCopy[0] == RESET_REG)
    {
      digitalWrite(RST, LOW);
      delayMicroseconds(100);
      digitalWrite(RST, HIGH);
    }

    bytesToSend = 0; // Done
  }
}

// function that executes whenever data is received from the controller
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  while(Wire.available())
  {
    if (counter <= maxCount)
      i2cBuffer[counter++] = Wire.read(); // Store in buffer
    else
      Wire.read(); // Discard
  }
}

// function that executes whenever data is requested by the controller
// this function is registered as an event, see setup()
// write the BUSY state
void requestEvent()
{
  Wire.write(digitalRead(BUSY));
}
