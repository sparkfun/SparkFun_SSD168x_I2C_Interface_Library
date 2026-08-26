# Wiring

## Wiring for STM32G030 programming

You will need (e.g.) a ST ST-LINK V2 programmer

Connect the programmer 20-way (2x10 way) header to the STM32G030 dev board as follows:

| ST-LINK V2 | STM32G030 Dev Board |
|---|---|
| Pin 19 VSUPPLY & Pin 1 VREF | 3V3 |
| Pin 20 GND | GND |
| Pin 7 SWDIO | DIO |
| Pin 9 SWCLK | CLK |
| Pin 15 RESET | RST |

Join pin 19 to pin 1, and connect both to the dev board 3V3

Use (e.g.) ST STM32CubeProgrammer to program the [STM32G030_I2C_SPI_Bridge.ino.GENERIC_G030F6PX.bin](https://github.com/sparkfun/SparkFun_SSD168x_I2C_Interface_Library/blob/main/STM32G03/STM32G030_I2C_SPI_Bridge/STM32G030_I2C_SPI_Bridge.ino.GENERIC_G030F6PX.bin) firmware onto the STM32, starting at address 0x8000000.

## Wiring for Thing Plus USB-C demo

Connect the SparkFun Thing Plus - ESP32 WROOM (USB-C) to the STM32G030 dev board using a [Qwiic Jumper Cable](https://www.sparkfun.com/flexible-qwiic-cable-female-jumper-4-pin.html):

| Qwiic | STM32G030 Dev Board |
|---|---|
| Red 3V3 | 3V3 |
| Black GND | GND |
| Blue SDA | B7 |
| Yellow SCL | B3 (alternate of B6) |

Connect the STM32G030 dev board to the GoodDisplay DESPI-C02 as follows:

| STM32G030 Dev Board | DESPI-C02 |
| 3V3 | 3.3V |
| GND | GND |
| A7 (PICO) | SDI |
| A5 (SCK) | SCK |
| B0 (CS) | CS |
| A0 | D/C |
| A2 | RES |
| A1 | BUSY |

Set the DESPI-C02 sense switch to 2.2 Ohm

