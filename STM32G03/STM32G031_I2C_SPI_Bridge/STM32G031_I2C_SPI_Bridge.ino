/* STM32 (G031) I2C to SPI Bridge - for e-paper displays

By P.C. for SparkFun Electronics, May 6th 2026

Based loosely on example code from the STM32 Boards package

This code runs on an STM32G031 processor. Tested on the STM32G031K8T6 on the NUCLEO-G031K8 dev board

This code acts as a I2C to SPI bridge, to interface with SSD168x e-paper displays

Connections:
D4  (PA10) I2C SDA
D5  (PA9)  I2C SCL
D6  (PB0)  e-paper D/C#
D7  (PB2)  e-paper BUSY
D8  (PB8)  e-paper RST
D10 (PB9)  SPI CS
D11 (PB5)  SPI MOSI : data to e-paper
D12 (PB4)  SPI MISO : not used
D13 (PB3)  SPI SCK  : also LED_BUILTIN on the NUCLEO-G031K8 dev board

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
const int DC = PB_0;
const int BUSY = PB_2;
const int RST = PB_8;
const int CS = PB_9;

const SPISettings spiSettings = { 2000000, MSBFIRST, SPI_MODE0 };

static volatile uint8_t counter = 0;
const uint8_t maxCount = 200; // I2C receive is limited to 32 bytes. TODO: can this be increased?
static uint8_t i2cBuffer[maxCount + 1]; // i2cBuffer[0] contains the 'register' address
static uint8_t i2cBufferCopy[maxCount + 1];
static volatile uint8_t bytesToSend;

void setup()
{
  Serial.begin(115200); // start serial for output
  Serial.println("STM32 G031 I2C SPI Bridge");
  Serial.println("========================");
  Serial.println("D4  (PA10) I2C SDA");
  Serial.println("D5  (PA9)  I2C SCL");
  Serial.println("D6  (PB0)  e-paper D/C#");
  Serial.println("D7  (PB2)  e-paper BUSY");
  Serial.println("D8  (PB8)  e-paper RST");
  Serial.println("D10 (PB9)  SPI CS");
  Serial.println("D11 (PB5)  SPI MOSI : data to e-paper");
  Serial.println("D12 (PB4)  SPI MISO : not used");
  Serial.println("D13 (PB3)  SPI SCK  : also LED_BUILTIN");
  Serial.flush();

  pinMode(DC, OUTPUT);
  pinMode(BUSY, INPUT);
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(PB_5, OUTPUT);
  pinMode(PB_4, INPUT);
  pinMode(PB_3, OUTPUT);
  SPI.setMOSI(PB_5);
  SPI.setMISO(PB_4);
  SPI.setSCLK(PB_3);
  //SPI.setSSEL(PB_9);
  SPI.begin();

  SPI.beginTransaction(spiSettings); // Dummy transaction to set clock speed
  SPI.transfer(0x7F); // SSD168x NOP
  SPI.endTransaction();

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
