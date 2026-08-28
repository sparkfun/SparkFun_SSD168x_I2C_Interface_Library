// i2c_ssd1680.cpp
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

#include "i2c_ssd1680.h"

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

void I2cSsd1680::setupDefaults(void)
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
bool I2cSsd1680::init(void)
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
    m_nPages = m_viewport.width / kByteNBits; // width / number of pixels per byte.

    // Flag that we are initialized
    m_isInitialized = true;

    // setup e-paper device - needs m_isInitialized
    setupEpaperDevice(); // calls initBuffers which will call clearScreenBuffer

    // Perform a full update
    display();

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

bool I2cSsd1680::reset(void)
{
    // If we are not in an init state, just call init
    if (!m_isInitialized)
        return init();

    // Init internal/drawing buffers and device screen buffer
    initBuffers(); // Note: calls clearScreenBuffer

    // Perform a full update
    display();

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
// Method sends the init/setup commands to the OLED device, placing
// it in a state for use by this driver/library.

void I2cSsd1680::setupEpaperDevice(void)
{
    // Start the device setup - sending commands to device. See command defs in
    // header, and device datasheet

    sendDevReset();

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
    buffer[0] = 0;
    buffer[1] = (m_viewport.width / 8) - 1;
    sendDevCommand( kCmdSsd1680SetRamPosX, buffer, 2 );

    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = m_viewport.height - 1;
    buffer[3] = (m_viewport.height - 1) >> 8;
    sendDevCommand( kCmdSsd1680SetRamPosY, buffer, 4 );

    buffer[0] = m_viewport.height - 1;
    buffer[1] = (m_viewport.height - 1) >> 8;
    buffer[2] = 0;
    sendDevCommand( kCmdSsd1680DriverOutput, buffer, 3 );

    // GoodDisplay GDEM0097T61
    // Update the Display Option to disable Mode 2 ping-pong
    // Display defaults:  0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
    // Disable ping-pong: 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    //uint8_t displayOption[10] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //sendDevCommand(kCmdSsd1680DisplayOption, &displayOption[0], 10);

    // **Update in Y direction**, Y increment, X increment
    sendDevCommand(kCmdSsd1680DataEntryMode, 0x07);

    initBuffers(); // clear graphics and screen buffer
}
////////////////////////////////////////////////////////////////////////////////////
// setCommBus()
//
// Method to set the bus object that is used to communicate with the device
//
// TODO -  In the *future*, generalize to match SDK

void I2cSsd1680::setCommBus(QwEpI2C &theBus, uint8_t id_bus)
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
void I2cSsd1680::setBuffer(uint8_t *pBuffer)
{
    if (pBuffer)
        m_pBuffer = pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////
// clearScreenBuffer()
//
// Clear out all the on-device memory.
//
void I2cSsd1680::clearScreenBuffer(void)
{
    // Clear out the **visible** screen buffer on the device
    uint8_t emptyPage[m_viewport.height];
    memset(emptyPage, COLOR_OFF, m_viewport.height); // OFF = 0. Becomes White due to inversion

    for (int i = 0; i < m_nPages; i++)
    {
        setScreenBufferAddress(i, 0, m_viewport.height - 1); // start of page

        sendDevCommand(kCmdSsd1680WriteRamBW, (uint8_t *)emptyPage, m_viewport.height); // clear out page

        delay(1);

        // Repeat for Red RAM - used as the background / base map for partial updates
        
        setScreenBufferAddress(i, 0, m_viewport.height - 1); // start of page

        sendDevCommand(kCmdSsd1680WriteRamRed, (uint8_t *)emptyPage, m_viewport.height); // clear out page

        delay(1);
    }
}
////////////////////////////////////////////////////////////////////////////////////
// initBuffers()
//
// Will clear the local graphics buffer, and the devices screen buffer. Also
// resets page state descriptors to a "clean" state.

void I2cSsd1680::initBuffers(void)
{
    int i,j;

    // clear out the local graphics buffer
    if (m_pBuffer)
        memset(m_pBuffer, COLOR_OFF, m_viewport.height * m_viewport.width / kByteNBits);

    // Set page descs to "clean" state
    for (j = 0; j < kNumRamBanksSSD168x; j++)
    {
        for (i = 0; i < m_nPages; i++)
        {
            pageSetClean(m_pageState[j][i]);
            pageSetClean(m_pageErase[j][i]);
            m_pendingErase[j][i] = false;
        }
    }

    // clear out the screen buffer
    clearScreenBuffer();
}
////////////////////////////////////////////////////////////////////////////////////
// resendGraphics()
//
// Re-send the region in the graphics buffer (local) that contains drawn
// graphics. This region is defined by the contents of the m_pageErase
// descriptors.
//
// Copy these to the page state, and call display
//

void I2cSsd1680::resendGraphics(void)
{
    // Set the page state dirty bounds to the bounds of erase state
    for (int i = 0; i < m_nPages; i++)
        m_pageState[0][i] = m_pageErase[0][i];

    display(); // push bits to screen buffer
}

////////////////////////////////////////////////////////////////////////////////////
// deepSleep()
//
// Used to set the power of the screen.
// Careful now! Display needs a hardware reset to wake from deep sleep...

void I2cSsd1680::deepSleep(bool mode2)
{
    if (!m_isInitialized)
        return;

    sendDevCommand(kCmdSsd1680DeepSleep, mode2 ? 0x03 : 0x01); // Deep Sleep Mode 2/1
}

////////////////////////////////////////////////////////////////////////////////////
// isBusy()
//
// Used to read the state of the SSD168x BUSY pin (via I2C)

bool I2cSsd1680::isBusy(void)
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

void I2cSsd1680::erase(void)
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

        // Copy the current page info into previous
        m_pageState[1][i] = m_pageState[0][i];
        m_pageErase[1][i] = m_pageErase[0][i];
        m_pendingErase[1][i] = m_pendingErase[0][i];

        pageCheckBoundsDesc(m_pageState[0][i], m_pageErase[0][i]);

        // if this page is clean, there is nothing to update
        if (pageIsClean(m_pageState[0][i]))
            continue;

        // clear out memory that is dirty on this page
        memset(m_pBuffer + i * m_viewport.height + m_pageState[0][i].min, COLOR_OFF,
               m_pageState[0][i].max - m_pageState[0][i].min + 1); // add one b/c values are 0 based

        // clear out any pending dirty range for this page - it's erased
        pageSetClean(m_pageState[0][i]);

        // Indicate that the data transfer to the device should include the erase
        // region
        m_pendingErase[0][i] = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
// draw_pixel()
//
// Used to set a pixel in the graphics buffer - uses the current write operator
// function
//

void I2cSsd1680::drawPixel(uint8_t x, uint8_t y, uint8_t clr)
{
    // quick sanity check on range
    if (x >= m_viewport.width || y >= m_viewport.height)
        return; // out of bounds

    uint8_t bit = gfx_byte_bits[mod_byte(x)];
    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op
    curROP(m_pBuffer + y + x / kByteNBits * m_viewport.height, // pixel offset
                       (clr == COLOR_ON ? bit : 0), bit);      // which bit to set / clear in byte

    pageCheckBounds(m_pageState[0][x / kByteNBits],
                    y); // update dirty range for page
}
////////////////////////////////////////////////////////////////////////////////////
// draw_line_vert()
//
// Fast vertical line drawing routine
//

void I2cSsd1680::drawLineVert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr)
{
    // Basically we set a bit within a range in a page of our graphics buffer.

    // in range
    if (x0 >= m_viewport.width)
        return;

    if (y0 > y1)
        swap_int(y0, y1);

    if (y1 >= m_viewport.height)
        y1 = m_viewport.height - 1;

    uint8_t bit = gfx_byte_bits[mod_byte(x0)];   // bit to set
    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op

    // Get the start of this line in the graphics buffer
    uint8_t *pBuffer = m_pBuffer + y0 + x0 / kByteNBits * m_viewport.height;

    // walk up x and set the target pixel using the pixel operator function
    for (int i = y0; i <= y1; i++, pBuffer++)
        curROP(pBuffer, (clr == COLOR_ON ? bit : 0), bit);

    // Mark the page dirty for the range drawn
    pageCheckBoundsRange(m_pageState[0][x0 / kByteNBits], y0, y1);
}
////////////////////////////////////////////////////////////////////////////////////
// draw_line_horz()
//
// Fast horizontal line drawing routine - also supports fast filled rects
//
void I2cSsd1680::drawLineHorz(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t clr)
{
    if (y0 >= m_viewport.height) // out of bounds
        return;

    // want an accending order
    if (x0 > x1)
        swap_int(x0, x1);

    // keep on screen
    if (x1 >= m_viewport.width)
        x1 = m_viewport.width - 1;

    uint8_t startBit, endBit, setBits;

    // Get the start and end pages we are writing to
    uint8_t page0 = x0 / kByteNBits;
    uint8_t page1 = x1 / kByteNBits;

    // loop over the pages. For each page determine the range of pixels
    // to set in the target page byte and then set them using the current
    // pixel operator function

    // Note: This function can also be used to draw filled rects - just iterate
    //       in the y direction. The base rect fill (in grBuffer) calls this
    //       method y1-y0 times, and each of those calls has some overhead. So
    //       just iterating over each page - y1-y0 times here - saves overhead
    //       costs.
    //
    //       To make this work, make sure y0 > y1. Also, this method is wired in
    //       as the draw_rect_filled entry in the draw interface. This is done
    //       above in the init process.

    int yinc;
    if (y0 > y1)
        swap_int(y0, y1);

    rasterOPsFn curROP = m_rasterOps[m_rop]; // current raster op

    for (int i = page0; i <= page1; i++)
    {
        startBit = mod_byte(x0); // start bit in this byte

        // last bit of this byte to set? Does the line end in this byte, or continue
        // on...
        endBit = x0 + kByteNBits - startBit > x1 ? mod_byte(x1) : kByteNBits - 1;

        // Set the bits from startBit to endBit
        setBits = (0xFF << ((kByteNBits - endBit) - 1)) >> startBit; // what bits are being set in this byte

        // set the bits in the graphics buffer using the current byte operator
        // function

        // Note - We iterate over y to fill in a rect if specified.
        for (yinc = y0; yinc <= y1; yinc++)
            curROP(m_pBuffer + i * m_viewport.height + yinc, (clr == COLOR_ON ? setBits : 0), setBits);

        x0 += endBit - startBit + 1; // increment x0 to next page

        pageCheckBoundsRange(m_pageState[0][i], y0,
                             y1); // mark dirty range in page desc
    }
}
////////////////////////////////////////////////////////////////////////////////////////
// draw_rect_fill()
//
// Does the actual drawing/logic

void I2cSsd1680::drawRectFilled(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height, uint8_t clr)
{
    uint8_t x1 = x0 + width - 1;
    uint8_t y1 = y0 + height - 1;

    // just call horz line
    drawLineHorz(x0, y0, x1, y1, clr);
}
////////////////////////////////////////////////////////////////////////////////////
// draw_bitmap()
//
// Draw a 8 bit encoded bitmap to the screen
//

void I2cSsd1680::drawBitmap(uint8_t x0, uint8_t y0, uint8_t dst_width, uint8_t dst_height, uint8_t *pBitmap,
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
    //   - For the SSD1680, the pages are vertical, so this gets gnarly...
    //     We can either read a column byte and set its bits in the appropriate
    //     page rows
    //     Or we can just scan each pixel in turn and set that pixel as needed...
    //     Scanning is slow, but I don't think bytes to rows would be much faster?

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

            pageCheckBoundsRange(m_pageState[0][(x0 + x) / kByteNBits], y0,
                                 y0 + dst_height - 1); // mark dirty range in page desc
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
// The positon is specified by page and row
//
// The system runs in "page mode" - data is streamed along a page, based
// on the set starting position.
//
// This class takes advantage of this to just write the "dirty" ranges in a
// page.
//

bool I2cSsd1680::setScreenBufferAddress(uint8_t page, uint8_t rowStart, uint8_t rowEnd)
{
    if (page >= m_nPages || rowStart >= m_viewport.height || rowEnd >= m_viewport.height)
        return false;

    uint8_t buffer[4];
    
    buffer[0] = page;
    buffer[1] = page;
    sendDevCommand( kCmdSsd1680SetRamPosX, buffer, 2 );

    buffer[0] = rowStart;
    buffer[1] = rowStart >> 8; // 0 (row is uint8_t)
    buffer[2] = rowEnd;
    buffer[3] = rowEnd >> 8; // 0 (row is uint8_t)
    sendDevCommand( kCmdSsd1680SetRamPosY, buffer, 4 );

    buffer[0] = page;
    sendDevCommand( kCmdSsd1680SetRamCounterX, buffer, 1 );

    buffer[0] = rowStart;
    buffer[1] = rowStart >> 8; // 0 (row is uint8_t)
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

void I2cSsd1680::display(bool partial)
{
    // Sending only the dirty areas is probably OK because init calls clearScreenBuffer
    // which clears both BW and Red RAM.

    bool displayUpdated = false;

    // Loop over our page descriptors - if a page is dirty, send the graphics
    // buffer dirty region to the device for the current page

    pageStateEp_t transferRange;

    for (int i = 0; i < m_nPages; i++)
    {
        // We keep the erase rect seperate from dirty rect. Make temp copy of
        // dirty rect page range, expand to include erase rect page range.

        transferRange = m_pageState[0][i];
        pageCheckBoundsDesc(transferRange, m_pageState[1][i]);

        // If an erase has happend, we need to transfer/include erase update range
        if (m_pendingErase[0][i])
            pageCheckBoundsDesc(transferRange, m_pageErase[0][i]);
        if (m_pendingErase[1][i])
            pageCheckBoundsDesc(transferRange, m_pageErase[1][i]);

        if (pageIsClean(transferRange)) // both dirty and erase range for this
                                        // page were null
        {
            m_pageState[1][i] = m_pageState[0][i]; // Copy current into previous
            m_pageErase[1][i] = m_pageErase[0][i];
            m_pendingErase[1][i] = m_pendingErase[0][i];
            m_pendingErase[0][i] = false; // no longer pending. Redundant?
            continue;                     // next
        }

        // Perform hardware reset - GoodDisplay code always does this - not sure if it is strictly necessary?
        sendDevReset(); // Hardware reset

        do {
            delay(10);
        }
        while (isBusy());

        // Set border - GoodDisplay code always does this
        if (partial)
            sendDevCommand( kCmdSsd1680WriteBorder, 0x80 ); // VCOM
        else
            sendDevCommand( kCmdSsd1680WriteBorder, 0x05 ); // Follow LUT1 (White)

        // set the start address to write the updated data to the devices screen
        // buffer
        setScreenBufferAddress(i, transferRange.min, transferRange.max);

        // send the dirty data to the device
        sendDevCommand(kCmdSsd1680WriteRamBW, m_pBuffer + (i * m_viewport.height) + transferRange.min, // this page start + min
                    transferRange.max - transferRange.min + 1); // dirty region max - min. Add 1 b/c 0 based

        delay(1); // Wait for I2C->SPI at 1MHz

        // If partial is not true, write the same data to the Red RAM so the SSD1680 can
        // diff it on the next partial write

        if (!partial)
        {
            // GoodDisplay only sets the RAM address and counters once...

            // send the dirty data to the device
            sendDevCommand(kCmdSsd1680WriteRamRed, m_pBuffer + (i * m_viewport.height) + transferRange.min, // this page start + min
                        transferRange.max - transferRange.min + 1); // dirty region max - min. Add 1 b/c 0 based

            delay(1); // Wait for I2C->SPI at 1MHz
        }

        m_pageState[1][i] = m_pageState[0][i]; // Copy current into previous
        m_pageErase[1][i] = m_pageErase[0][i];
        m_pendingErase[1][i] = m_pendingErase[0][i];

        // If we sent the erase bounds, zero out the erase bounds - this area is now
        // clear
        if (m_pendingErase[0][i])
        {
            m_pendingErase[0][i] = false; // no longer pending
            pageSetClean(m_pageErase[0][i]);
        }

        // add the just send dirty range (non erase rec)  to the erase rect
        pageCheckBoundsDesc(m_pageErase[0][i], m_pageState[0][i]); // TODO - CHECK THIS!

        // this page is no longer dirty - mark it  clean
        pageSetClean(m_pageState[0][i]);

        displayUpdated = true;
    }

    if (!partial || displayUpdated) // If some dirty pixels were sent, activate the display
    {
        sendDevCommand( kCmdSsd1680DisplayUpdateCtrl2, partial ? 0xFF : 0xF7 ); // DISPLAY with DISPLAY Mode 2 / 1
        sendDevCommand( kCmdSsd1680MasterActivate ); // Activate
    }
}

////////////////////////////////////////////////////////////////////////////////////
// Device communication methods
////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command to the device via the current bus object

void I2cSsd1680::sendDevCommand(uint8_t command)
{
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendSingleCommand, command);
}

////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command and value to the device via the current bus object.
//

void I2cSsd1680::sendDevCommand(uint8_t command, uint8_t value)
{
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendCommand, command);
    m_i2cBus->writeRegisterByte(m_i2cAddress, kDeviceSendData, value);
}

////////////////////////////////////////////////////////////////////////////////////
// sendDeviceCommand()
//
// send a single command and multiple values to the device via the current bus object.

void I2cSsd1680::sendDevCommand(uint8_t command, uint8_t *values, uint16_t n_values)
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

void I2cSsd1680::sendDevReset(void)
{
    m_i2cBus->writeRegister(m_i2cAddress, kDeviceSendReset);
}

////////////////////////////////////////////////////////////////////////////////////
// readDevStatus()
//
// read a byte from the device via the current bus object

uint8_t I2cSsd1680::readDevStatus(void)
{
    return m_i2cBus->readRegisterByte(m_i2cAddress);
}
