# SparkFun SSD168x I2C Interface Library
========================================

<table class="table table-hover table-striped table-bordered">
    <p> TODO: Add product photos and links </p>
</table>

This is an experimental library to control SSD1680/1 e-Paper displays via I2C, using a TI MSP430FR2433 as the I2C to SPI Bridge.

The MSP430FR2433 is configured as a I2C peripheral with two registers: Control (Register 0x00), and Data (Register 0x01).
All data written to Register 0x00 is bridged to SPI with the D/C# pin held low.
All data written to Register 0x01 is bridged to SPI with the D/C# pin held high.

Repository Contents
-------------------

* **/documents** - Data sheets, additional product information
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

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.

_<COLLABORATION CREDIT>_
