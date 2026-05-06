# SparkFun SSD168x I2C Interface Library
========================================

<table class="table table-hover table-striped table-bordered">
    <p> TODO: Add product photos and links </p>
</table>

A library to support SSD1680/1 e-paper displays on I2C, using a I2C-SPI Bridge.

The I2C to SPI Bridge is configured as a I2C peripheral with three registers: Control (Register 0x00), Data (Register 0x01) and Reset (Register 0x02).
* All data written to Register 0x00 is bridged to SPI with the D/C# pin held low.
* All data written to Register 0x01 is bridged to SPI with the D/C# pin held high.
* A write to Register 0x02 causes RST to be pulled low briefly.
* I2C reads return bytes containing the e-paper BUSY flag in the LSB.

The MSP430FR2433 and STM32G03 folders contain example I2C-SPI Bridge firmware, tested on the:
* TI MSP430FR2433 on the MSP-EXP430FR2433 dev board (using TI Code Composer Studio)
* ST STM32G031K8T6 on the NUCLEO-G031K8 dev board (using the Arduino IDE and the STM32 Arduino Board package)

Repository Contents
-------------------

* **/MSP430FR2433** - Example I2C-SPI Bridge firmware for the TI MSP430FR2433
* **/STM32G03** - Example I2C-SPI Bridge firmware for the ST STM32G031K8T6
* **/examples** - Example code 
* **/src** - Source code

Documentation
--------------
* **[GitHub Repo](https://github.com/sparkfun/TODO)** - TODO: Update URL and description
* **[Hookup Guide](http://docs.sparkfun.com/TODO/)** - TODO: Update URL and description

License Information
-------------------

This product is _**open source**_! 

Please review the LICENSE.md file for license information. 

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://community.sparkfun.com/c/components-and-accessories/displays/29).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.

_<COLLABORATION CREDIT>_
