// i2c_ssd1680_rotated.cpp
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

#include "i2c_ssd1680_rotated.h"

/////////////////////////////////////////////////////////////////////////////
// Class that implements graphics support for devices that use the SSD1680
//

//////////////////////////////////////////////////////////////////////////////////
// Communication
//
// When communicating with the device, you either send commands or data. Define
// our codes for these two options - these are basically i2c registers/offsets.
// Note: these are specific to our I2C-SPI Bridge
//
#define kDeviceSendSingleCommand 0x00
#define kDeviceSendCommand 0x01
#define kDeviceSendData 0x02
#define kDeviceSendFinalData 0x03
#define kDeviceSendReset 0x04

////////////////////////////////////////////////////////////////////////////////////
// Pixel write/set operations
//
// Using LAMBDAs to create fast raster write/set operations. Using this pattern
// eleminates the need for switch/if statements in each draw routine. This is
// basically classic ROPs'
//
// NOTE - the order in the arrays is based on grEpRasterOp_t enum
//
// The Graphic operator functions (ROPS)
//      - Copy      - copy the pixel value in to the buffer (default)
//      - Not Copy  - copy the not of the pixel value to buffer
//      - Not       - Set the buffer value to not it's current value
//      - XOR       - XOR of color and current pixel value
//      - Off       - Set value to always be Off (White on e-paper due to inversion)
//      - On        - set value to always be On (Black on e-paper due to inversion)

typedef void (*rasterOPsFn)(uint8_t *dest, uint8_t src, uint8_t mask);

static const rasterOPsFn m_rasterOps[] = {
    // COPY
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = (~mask & *dst) | (src & mask); },
    // NOT COPY
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = (~mask & *dst) | ((~src) & mask); },
    // NOT DEST
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = (~mask & *dst) | ((~(*dst)) & mask); },
    // XOR
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = (~mask & *dst) | ((*dst ^ src) & mask); },
    // Always Off
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = ~mask & *dst; },
    // Always On
    [](uint8_t *dst, uint8_t src, uint8_t mask) -> void { *dst = mask | *dst; },
};

////////////////////////////////////////////////////////////////////////////////////
// setup defaults - called from constructors
//
// Just a bunch of member variable inits

void I2cSsd1680Rotated::setupDefaults(void)
{
    default_address = {0};
    m_pBuffer = {nullptr};
    m_color = {1};
    m_rop = {grEpROPCopy};
    m_i2cBus = {nullptr};
    m_i2cAddress = {0};
    m_isInitialized = {false};
}
////////////////////////////////////////////////////////////////////////////////////
// init()
//
// Called by user when the device/system is up and ready to be "initialized."
//
// This implementation performs the basic setup for the SSD1680 device
//
// The startup sequence is as follows:
//
//      - Make sure a device is connected
//      - Call super class
//      - Shutdown the device (display off), initial device setup, turn on
//      device
//      - Init the local graphics buffers/system
//
// When this method is complete, the driver and device are ready for use
//
bool I2cSsd1680Rotated::init(void)
{
    if (m_isInitialized)
        return true;

    //  do we have a bus yet? Buffer? Note - buffer is set by subclass of this
    //  object
    if (!m_i2cBus || !m_i2cAddress || !m_pBuffer)
        return false;

    // Is the device connected?
    if (!m_i2cBus->ping(m_i2cAddress))
        return false;

    // Super-class
    if (!this->QwEpGrBufferDevice::init())
        return false; // something isn't right

    // Number of pages used for this device?
    // Note: we are rotated. Height defines the pages, not width
    m_nPages = m_viewport.height / kByteNBits; // width / number of pixels per byte.

    // Flag that we are initialized
    m_isInitialized = true;

    // setup e-paper device - needs m_isInitialized
    setupEpaperDevice(); // calls initBuffers which will call clearScreenBuffer

    // Perform a full update
    display(false, false);

    do {
        delay(10);
    }
    while (isBusy());

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// reset()
//
// Wake and reset the device
//
// Returns true on success, false on failure

bool I2cSsd1680Rotated::reset(void)
{
    // If we are not in an init state, just call init
    if (!m_isInitialized)
        return init();

    // Init internal/drawing buffers and device screen buffer
    initBuffers(); // Note: calls clearScreenBuffer

    // Perform a full update
    display(false, false);

    // User must check isBusy externally
    // do {
    //     delay(10);
    // }
    // while (isBusy());

    return true;
}
////////////////////////////////////////////////////////////////////////////////////
// Configuration API
//
// This allows sub-classes to setup for their device, while preserving
// encapsulation.
//
// These should be called/set before calling init
//
// For details of each of these settings -- see the datasheet
//


////////////////////////////////////////////////////////////////////////////////////
// setupEpaperDevice()
//
// PRIVATE
//
// Method sends the init/setup commands to the e-paper device, placing
// it in a state for use by this driver/library.

void I2cSsd1680Rotated::setupEpaperDevice(void)
{
    // Start the device setup - sending commands to device. See command defs in
    // header, and device datasheet

    sendDevReset(); // Hardware reset

    do {
        delay(10);
    }
    while (isBusy());

    for (int i = 0; i < numSsd1680InitCodeEntries; i++)
    {
        if (ssd1680InitCode[i].numFollowingBytes == 0)
            sendDevCommand(ssd1680InitCode[i].command);
        else if (ssd1680InitCode[i].numFollowingBytes == 1)
            sendDevCommand(ssd1680InitCode[i].command, ssd1680InitCode[i].followingBytes[0]);
        else
            sendDevCommand(ssd1680InitCode[i].command,
                        (uint8_t *)&ssd1680InitCode[i].followingBytes[0],
                        ssd1680InitCode[i].numFollowingBytes);

        if (ssd1680InitCode[i].delayAfter)
            delay(ssd1680InitCode[i].delayDuration);

        if (ssd1680InitCode[i].checkBusyAfter)
        {
            do {
                delay(10);
            } while (isBusy());
        }
    }

    uint8_t buffer[4];
    // Note: we are rotated: X is height...
    buffer[0] = 0;
    buffer[1] = (m_viewport.height / 8) - 1;
    sendDevCommand( kCmdSsd1680SetRamPosX, buffer, 2 );

    // Note: we are rotated: Y is width...
    buffer[0] = m_viewport.width - 1;
    buffer[1] = (m_viewport.width - 1) >> 8;
    buffer[2] = 0;
    buffer[3] = 0;
    sendDevCommand( kCmdSsd1680SetRamPosY, buffer, 4 );

    buffer[0] = m_viewport.width - 1;
    buffer[1] = (m_viewport.width - 1) >> 8;
    buffer[2] = 0;
    sendDevCommand( kCmdSsd1680DriverOutput, buffer, 3 );

    // GoodDisplay GDEM0097T61
    // Update the Display Option to disable Mode 2 ping-pong
    // Careful! This needs a power cycle or an overwrite to restore
    // Display defaults:  0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
    // Disable ping-pong: 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    //uint8_t displayOption[10] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //sendDevCommand(kCmdSsd1680DisplayOption, &displayOption[0], 10);

    // **Update in Y direction**, Y decrement, X increment
    sendDevCommand(kCmdSsd1680DataEntryMode, 0b00000101);

    initBuffers(); // clear graphics and screen buffer
}
////////////////////////////////////////////////////////////////////////////////////
// setCommBus()
//
// Method to set the bus object that is used to communicate with the device
//
// TODO -  In the *future*, generalize to match SDK

void I2cSsd1680Rotated::setCommBus(QwEpI2C &theBus, uint8_t id_bus)
{
    m_i2cBus = &theBus;
    m_i2cAddress = id_bus;
}

////////////////////////////////////////////////////////////////////////////////////
// setBuffer()
//
// Protected method - used by sub-class to set the graphics buffer array.
//
// The subclass knows the size of the specific device, so it statically defines
// the graphics buffer array. The buffer is often set in the subclasses
// on_initialize() method.
//
//
void I2cSsd1680Rotated::setBuffer(uint8_t *pBuffer)
{
    if (pBuffer)
        m_pBuffer = pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////
// clearScreenBuffer()
//
// PRIVATE
//
// Clear out all the on-device memory.
//
// The GoodDisplay code example:
//  Sets the RAM X address start / end - for the full display
//  Sets the RAM Y address start / end - for the full display
//  Sets the RAM X counter
//  Sets the RAM Y counter
//  Writes all 184*88/8 = 2024 bytes to BW RAM
//  Writes all 2024 bytes to RED RAM
//  Note: the RAM address and counter are not re-set between the two writes
//
void I2cSsd1680Rotated::clearScreenBuffer(void)
{
    // Clear out the **visible** screen buffer on the device
    uint8_t emptyBuffer[m_viewport.height * m_viewport.width / kByteNBits];
    memset(emptyBuffer, COLOR_OFF, m_viewport.height * m_viewport.width / kByteNBits); // OFF = 0. Becomes White due to inversion

    // We can't use setScreenBufferAddress here. We need to use real addresses

    uint8_t buffer[4];
    
    buffer[0] = 0;
    buffer[1] = (m_viewport.height / 8) - 1;
    sendDevCommand( kCmdSsd1680SetRamPosX, buffer, 2 );

    // We are using Y decrement mode. Set Y to max, min
    buffer[0] = (m_viewport.width - 1) & 0xFF;
    buffer[1] = (m_viewport.width - 1) >> 8;
    buffer[2] = 0;
    buffer[3] = 0;
    sendDevCommand( kCmdSsd1680SetRamPosY, buffer, 4 );

    buffer[0] = 0;
    sendDevCommand( kCmdSsd1680SetRamCounterX, buffer, 1 );

    // We are using Y decrement mode. Set Y to the max
    buffer[0] = (m_viewport.width - 1) & 0xFF;
    buffer[1] = (m_viewport.width - 1) >> 8;
    sendDevCommand( kCmdSsd1680SetRamCounterY, buffer, 2 );

    // Clear BW RAM
    sendDevCommand(kCmdSsd1680WriteRamBW, (uint8_t *)emptyBuffer, (uint16_t)m_viewport.height * (uint16_t)m_viewport.width / kByteNBits);
    delay(1);

    // Clear RED RAM
    sendDevCommand(kCmdSsd1680WriteRamRed, (uint8_t *)emptyBuffer, (uint16_t)m_viewport.height * (uint16_t)m_viewport.width / kByteNBits);
    delay(1);
}

////////////////////////////////////////////////////////////////////////////////////
// initBuffers()
//
// PRIVATE
//
// Will clear the local graphics buffer, and the devices screen buffer. Also
// resets page state descriptors to a "clean" state.

void I2cSsd1680Rotated::initBuffers(void)
{
    int i,j;

    // clear out the local graphics buffer
    if (m_pBuffer)
        memset(m_pBuffer, COLOR_OFF, m_viewport.width * m_viewport.height / kByteNBits);

    // Set page descs to "clean" state
    for (i = 0; i < m_nPages; i++)
    {
        pageSetClean(m_pageState[i]);
        pageSetClean(m_pageErase[i]);
        pageSetClean(m_pagePrevious[i]);
        m_pendingErase[i] = false;
    }

    // clear out the screen buffer
    clearScreenBuffer();
}

////////////////////////////////////////////////////////////////////////////////////
// deepSleep()
//
// Used to set the power of the screen.
// Careful now! Display needs a hardware reset to wake from deep sleep...

void I2cSsd1680Rotated::deepSleep(bool mode2)
{
    if (!m_isInitialized)
        return;

    sendDevCommand(kCmdSsd1680DeepSleep, mode2 ? 0x03 : 0x01); // Deep Sleep Mode 2/1
}

////////////////////////////////////////////////////////////////////////////////////
// isBusy()
//
// Used to read the state of the SSD168x BUSY pin (via I2C)

bool I2cSsd1680Rotated::isBusy(void)
{
    if (!m_isInitialized)
        return false;

    return (readDevStatus() & 0x01);
}

////////////////////////////////////////////////////////////////////////////////////
// Drawing Methods
////////////////////////////////////////////////////////////////////////////////////
// erase()
//
// Erase the graphics that are on screen and anything that's been draw but
// haven't been sent to the screen.
//

void I2cSsd1680Rotated::erase(void)
{
    if (!m_pBuffer)
        return;

    // Cleanup the dirty parts of each page in the graphics buffer.
    for (uint8_t i = 0; i < m_nPages; i++)
    {
        // m_pageState
        // The current "dirty" areas of the graphics [local] buffer.
        // Areas that haven't been sent to the screen/device but are
        // "dirty"
        //
        // Add the areas with pixels set and have been sent to the
        // device - this is the contents of m_pageErase

        pageCheckBoundsDesc(m_pageState[i], m_pageErase[i]);

        // if this page is clean, there is nothing to update
        if (pageIsClean(m_pageState[i]))
            continue;

        // clear out memory that is dirty on this page
        // Here, dirty min is left, dirty max is right. I.e. standard X coordinates
        // When we write to the display, these become Y...
        memset(m_pBuffer + i * m_viewport.width + m_pageState[i].min, COLOR_OFF,
               m_pageState[i].max - m_pageState[i].min + 1); // add one b/c values are 0 based

        // clear out any pending dirty range for this page - it's erased
        pageSetClean(m_pageState[i]);

        // Indicate that the data transfer to the device should include the erase
        // region
        m_pendingErase[i] = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
// draw_pixel()
//
// Used to set a pixel in the graphics buffer - uses the current write operator
// function
//

void I2cSsd1680Rotated::drawPixel(uint8_t x, uint8_t y, uint8_t clr)
{
    // quick sanity check on range
    if (x >= m_viewport.width || y >= m_viewport.height)
        return; // out of bounds

    uint8_t bit = gfx_byte_bits[mod_byte(y)];
    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op
    curROP(m_pBuffer + x + y / kByteNBits * m_viewport.width, // pixel offset
                       (clr == COLOR_ON ? bit : 0), bit);     // which bit to set / clear in byte

    pageCheckBounds(m_pageState[y / kByteNBits],
                    x); // update dirty range for page
}
////////////////////////////////////////////////////////////////////////////////////
// draw_line_horz()
//
// Fast horizontal line drawing routine
//

void I2cSsd1680Rotated::drawLineHorz(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr)
{
    // Basically we set a bit within a range in a page of our graphics buffer.

    // want an ascending order
    if (y0 > y1)
        swap_int(y0, y1);

    // in range
    if (y0 >= m_viewport.height)
        return;

    // want an ascending order
    if (x0 > x1)
        swap_int(x0, x1);

    if (x0 >= m_viewport.width)
        return;

    if (x1 >= m_viewport.width)
        x1 = m_viewport.width - 1;

    uint8_t bit = gfx_byte_bits[mod_byte(y0)];   // bit to set
    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op

    // Get the start of this line in the graphics buffer
    uint8_t *pBuffer = m_pBuffer + x0 + y0 / kByteNBits * m_viewport.width;

    // walk across and set the target pixel using the pixel operator function
    for (int i = x0; i <= x1; i++, pBuffer++)
        curROP(pBuffer, (clr == COLOR_ON ? bit : 0), bit);

    // Mark the page dirty for the range drawn
    pageCheckBoundsRange(m_pageState[y0 / kByteNBits], x0, x1);
}

////////////////////////////////////////////////////////////////////////////////////
// draw_line_vert()
//
// Fast vertical line drawing routine - also supports fast filled rects
//
void I2cSsd1680Rotated::drawLineVert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr)
{
    // want an ascending order
    if (x0 > x1)
        swap_int(x0, x1);

    if (x0 >= m_viewport.width) // out of bounds
        return;

    if (x1 >= m_viewport.width) // out of bounds
        x1 = m_viewport.width - 1;

    // want an ascending order
    if (y0 > y1)
        swap_int(y0, y1);

    // keep on screen
    if (y0 >= m_viewport.height)
        return;

    if (y1 >= m_viewport.height)
        y1 = m_viewport.height - 1;

    uint8_t startBit, endBit, setBits;

    // Get the start and end pages we are writing to
    uint8_t page0 = y0 / kByteNBits;
    uint8_t page1 = y1 / kByteNBits;

    // loop over the pages. For each page determine the range of pixels
    // to set in the target page byte and then set them using the current
    // pixel operator function

    // Note: This function can also be used to draw filled rects - just iterate
    //       in the y direction. The base rect fill (in grBuffer) calls this
    //       method x1-x0 times, and each of those calls has some overhead. So
    //       just iterating over each page - x1-x0 times here - saves overhead
    //       costs.
    //
    //       To make this work, make sure x0 > x1. Also, this method is wired in
    //       as the draw_rect_filled entry in the draw interface. This is done
    //       above in the init process.

    int xinc;

    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op

    for (int i = page0; i <= page1; i++)
    {
        startBit = mod_byte(y0); // start bit in this byte

        // last bit of this byte to set? Does the line end in this byte, or continue
        // on...
        endBit = y0 + kByteNBits - startBit > y1 ? mod_byte(y1) : kByteNBits - 1;

        // Set the bits from startBit to endBit
        setBits = (0xFF << ((kByteNBits - endBit) - 1)) >> startBit; // what bits are being set in this byte

        // set the bits in the graphics buffer using the current byte operator
        // function

        // Note - We iterate over y to fill in a rect if specified.
        for (xinc = x0; xinc <= x1; xinc++)
            curROP(m_pBuffer + i * m_viewport.width + xinc, (clr == COLOR_ON ? setBits : 0), setBits);

        y0 += endBit - startBit + 1; // increment x0 to next page

        pageCheckBoundsRange(m_pageState[i], x0,
                             x1); // mark dirty range in page desc
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// draw_rect_fill()
//
// Does the actual drawing/logic

void I2cSsd1680Rotated::drawRectFilled(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height, uint8_t clr)
{
    uint8_t x1 = x0 + width - 1;
    uint8_t y1 = y0 + height - 1;

    // just call vert line
    drawLineVert(x0, y0, x1, y1, clr);
}

////////////////////////////////////////////////////////////////////////////////////
// draw_bitmap()
//
// Draw a 8 bit encoded bitmap to the screen
//

void I2cSsd1680Rotated::drawBitmap(uint8_t x0, uint8_t y0, uint8_t dst_width, uint8_t dst_height, uint8_t *pBitmap,
                             uint8_t bmp_width, uint8_t bmp_height)
{
    // some simple checks
    if (x0 >= m_viewport.width || y0 >= m_viewport.height || !bmp_width || !bmp_height)
        return;

    // Bounds check
    if (x0 + dst_width > m_viewport.width) // out of bounds
        dst_width = m_viewport.width - x0;

    if (bmp_width < dst_width)
        dst_width = bmp_width;

    if (y0 + dst_height > m_viewport.height) // out of bounds
        dst_height = m_viewport.height - y0;

    if (bmp_height < dst_height)
        dst_height = bmp_height;

    // The Plan:
    //   - The BMP data is arranged in columns which made it easier when copying
    //     into OLED horizontal pages
    //   - Here we are rotated, so we are also working with columns but the bit
    //     order is reversed. In the BMP, the LSB is top-most. On SSD1680, MSB
    //     is top-most... What to do? Do we use the optimized two-byte method
    //     from the OLED library and remember to correct the bit orientation.
    //     Or should we just scan each pixel in turn and set that pixel as needed...
    //     For now, let's scan. Our future selves can add the two-byte method
    //     if desired.

    for (uint16_t y = 0; y < dst_height; y++)
    {
        uint16_t row = y / kByteNBits;
        for (uint16_t x = 0; x < dst_width; x++)
        {
            uint16_t bytePtr = x + (row * bmp_width);
            uint8_t bitInByte = y % kByteNBits;
            uint8_t bitMask = gfx_byte_bits[(kByteNBits - 1) - bitInByte];
            uint8_t theByte = pBitmap[bytePtr];
            uint8_t color = (theByte & bitMask) ? COLOR_ON : COLOR_OFF;
            drawPixel(x0 + x, y0 + y, color);

            pageCheckBoundsRange(m_pageState[(y0 + y) / kByteNBits], x0,
                                 x0 + dst_width - 1); // mark dirty range in page desc
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////
// Device Update Methods
////////////////////////////////////////////////////////////////////////////////////
// setScreenBufferAddress()
//
// Sets the target screen buffer address for graphics buffer transfer to the
// device.
//
// The positon is specified by page and column
//
// The system runs in "page mode" - data is streamed along a page, based
// on the set starting position.
//
// This class takes advantage of this to just write the "dirty" ranges in a
// page.
//
// Remember we are rotated and using Y-decrement

bool I2cSsd1680Rotated::setScreenBufferAddress(uint8_t page, uint8_t columnStart, uint8_t columnEnd)
{
    if (page >= m_nPages || columnStart >= m_viewport.width || columnEnd >= m_viewport.width)
        return false;

    uint8_t buffer[4];
    
    buffer[0] = page;
    buffer[1] = page;
    sendDevCommand( kCmdSsd1680SetRamPosX, buffer, 2 );

    buffer[0] = (m_viewport.width - 1) - columnStart;
    buffer[1] = ((m_viewport.width - 1) - columnStart) >> 8; // 0 (column is uint8_t)
    buffer[2] = (m_viewport.width - 1) - columnEnd;
    buffer[3] = ((m_viewport.width - 1) - columnEnd) >> 8; // 0 (column is uint8_t)
    sendDevCommand( kCmdSsd1680SetRamPosY, buffer, 4 );

    buffer[0] = page;
    sendDevCommand( kCmdSsd1680SetRamCounterX, buffer, 1 );

    buffer[0] = (m_viewport.width - 1) - columnStart;
    buffer[1] = ((m_viewport.width - 1) - columnStart) >> 8; // 0 (column is uint8_t)
    sendDevCommand( kCmdSsd1680SetRamCounterY, buffer, 2 );

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// display()
//
// Send the "dirty" areas of the graphics buffer to the device's screen buffer.
// Only send the areas that need to be updated. The update region is based on
// new graphics to display, and any currently displayed items that need to be
// erased.
//
// The dirty areas are sent to both BW and RED RAM
//
// The GoodDisplay code example:
//  Sets the border waveform to: 0x05 for a full update; 0x80 for a partial update
//  Performs a hardware reset before each region is sent
//  Sets the RAM X address start / end
//  Sets the RAM Y address start / end
//  Sets the RAM X counter
//  Sets the RAM Y counter
//  For a partial update, each region is sent to BW RAM only
//  For a partial update, display update control 2 is set to 0xFF
//  For a full update, display update control 2 is set to 0xF7
//  After Master Activation, display is busy for: ~2.2s full update, ~0.5s partial update

void I2cSsd1680Rotated::display(bool partial, bool dirtyOnly)
{
    // Sending only the dirty areas is probably OK because init calls clearScreenBuffer
    // which clears both BW and Red RAM.

    bool displayUpdated = false;

    // Loop over our page descriptors - if a page is dirty, send the graphics
    // buffer dirty region to the device for the current page

    pageStateEp_t transferRange;

    for (int i = 0; i < m_nPages; i++)
    {
        if (dirtyOnly)
        {
            // We keep the erase rect seperate from dirty rect. Make temp copy of
            // dirty rect page range, expand to include erase rect page range.

            transferRange = m_pageState[i];

            // If an erase has happend, we need to transfer/include erase update range
            if (m_pendingErase[i])
                pageCheckBoundsDesc(transferRange, m_pageErase[i]);

            // Expand to include the previous range
            pageCheckBoundsDesc(transferRange, m_pagePrevious[i]);

            if (pageIsClean(transferRange)) // dirty, erase and previous range for this
                                            // page were null
            {
                m_pendingErase[i] = false; // Ensure pending is clear. Redundant?
                continue;                  // next
            }
        }
        else
        {
            // Send everything... Belts and suspenders...
            transferRange.max = m_viewport.width - 1;
            transferRange.min = 0;
        }

        // Perform hardware reset - GoodDisplay code always does this - not sure if it is strictly necessary?
        sendDevReset(); // Hardware reset

        do {
            delay(10);
        }
        while (isBusy());

        // Set border - GoodDisplay code always does this
        if (partial)
            sendDevCommand( kCmdSsd1680WriteBorder, 0x80 ); // VBD = VCOM
        else
            sendDevCommand( kCmdSsd1680WriteBorder, 0x05 ); // Follow LUT1 (White)

        // set the start address to write the updated data to the devices screen
        // buffer
        //Serial.printf("setScreenBufferAddress(%d, %d, %d)\r\n", i, transferRange.min, transferRange.max);
        setScreenBufferAddress(i, transferRange.min, transferRange.max);

        // send the dirty data to the device
        //Serial.printf("sendDevCommand(%d, %p, %d)\r\n", kCmdSsd1680WriteRamBW, m_pBuffer + (i * m_viewport.width) + transferRange.min,
        //            transferRange.max - transferRange.min + 1);
        sendDevCommand(kCmdSsd1680WriteRamBW, m_pBuffer + (i * m_viewport.width) + transferRange.min, // this page start + min
                    transferRange.max - transferRange.min + 1); // dirty region max - min. Add 1 b/c 0 based

        delay(1); // Wait for I2C->SPI at 1MHz

        // If partial is not true, write the same data to the Red RAM so the SSD1680 can
        // diff it on the next partial write

        if (!partial)
        {
            // GoodDisplay only sets the RAM address and counters once...

            // send the dirty data to the device
            sendDevCommand(kCmdSsd1680WriteRamRed, m_pBuffer + (i * m_viewport.width) + transferRange.min, // this page start + min
                        transferRange.max - transferRange.min + 1); // dirty region max - min. Add 1 b/c 0 based

            delay(1); // Wait for I2C->SPI at 1MHz
        }

        m_pagePrevious[i] = m_pageState[i]; // Copy current into previous
        if (m_pendingErase[i]) // Expand to include the erase area
            pageCheckBoundsDesc(m_pagePrevious[i], m_pageErase[i]);

        // If we sent the erase bounds, zero out the erase bounds - this area is now
        // clear
        if (m_pendingErase[i])
            pageSetClean(m_pageErase[i]);

        // add the just sent dirty range (non erase rec)  to the erase rect
        pageCheckBoundsDesc(m_pageErase[i], m_pageState[i]);

        // this page is no longer dirty - mark it  clean
        pageSetClean(m_pageState[i]);

        displayUpdated = true;
    }

    // If this is a full update, or some dirty pixels were sent, activate the display
    if (!partial || displayUpdated)
    {
        sendDevCommand( kCmdSsd1680DisplayUpdateCtrl2, partial ? 0xFF : 0xF7 ); // DISPLAY with DISPLAY Mode 2 / 1
        sendDevCommand( kCmdSsd1680MasterActivate ); // Activate
    }
    else
    {
        // If there was nothing new to display - no dirty pixels - then reset the SSD168x
        // just to wake it up. If it stays in deep sleep, it will hold BUSY high.
        sendDevReset(); // Hardware reset

        do {
            delay(1);
        }
        while (isBusy());
    }
}

////////////////////////////////////////////////////////////////////////////////////
// Device communication methods
////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command to the device via the current bus object

void I2cSsd1680Rotated::sendDevCommand(uint8_t command)
{
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendSingleCommand, command);
}

////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command and value to the device via the current bus object.
//

void I2cSsd1680Rotated::sendDevCommand(uint8_t command, uint8_t value)
{
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendCommand, command);
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendFinalData, value);
}

////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command and multiple values to the device via the current bus object.
// split the write between the Data register and the Final Data register

void I2cSsd1680Rotated::sendDevCommand(uint8_t command, uint8_t *values, uint16_t n_values)
{
    if (!values || n_values == 0)
        return;

    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendCommand, command);
    m_i2cBus->writeSplitRegisterRegion(m_i2cAddress, kDeviceSendData, kDeviceSendFinalData, values, n_values);
}

////////////////////////////////////////////////////////////////////////////////////
// sendDeviceReset()
//
// reset the device

void I2cSsd1680Rotated::sendDevReset(void)
{
    m_i2cBus->writeRegister(m_i2cAddress, kDeviceSendReset);
}

////////////////////////////////////////////////////////////////////////////////////
// readDevStatus()
//
// read a byte from the device via the current bus object

uint8_t I2cSsd1680Rotated::readDevStatus(void)
{
    return m_i2cBus->readRegisterByte(m_i2cAddress);
}
