/*---------------------------------------------------------*\
| X1RGBDevice.h                                             |
|                                                           |
|   Thermaltake Premium X1 RGB (USB 264A:3013) lighting     |
|   protocol over the vendor HID interface (MI_01,          |
|   usage page 0xFF00, 64-byte output reports, no report    |
|   IDs).                                                   |
|                                                           |
|   Protocol (from X1 RGB.exe v1.10.0, hardware-verified):  |
|     41 03                    enter host-control mode      |
|     C0 01 NN 00 [L R G B]*NN set NN (<=15) LEDs           |
|     C0 00 00 00 R G B        set all LEDs to one colour   |
|     C0 F0                    latch / show the new frame   |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define X1RGB_VID                   0x264A
#define X1RGB_PID                   0x3013
#define X1RGB_PID_BOOTLOADER        0x3012
#define X1RGB_PACKET_SIZE           64
#define X1RGB_LEDS_PER_PACKET       15

struct X1LedColor
{
    uint8_t     led;
    uint8_t     r;
    uint8_t     g;
    uint8_t     b;
};

class X1RGBDevice
{
public:
    explicit X1RGBDevice(const std::wstring& path);
    ~X1RGBDevice();

    /*-----------------------------------------------------*\
    | Returns HID interface paths of all connected X1 RGB   |
    | keyboards (lighting interface only)                   |
    \*-----------------------------------------------------*/
    static std::vector<std::wstring> Enumerate();

    bool        Open();
    void        Close();
    bool        IsOpen() const;

    bool        EnterHostMode();
    bool        SendFrame(const std::vector<X1LedColor>& leds);
    bool        SendFullColor(uint8_t r, uint8_t g, uint8_t b);

    std::string GetPathUTF8() const;

private:
    bool        Write(const uint8_t payload[X1RGB_PACKET_SIZE]);
    bool        Latch();

    std::wstring    path;
    void*           handle;         /* HANDLE */
    void*           write_event;    /* HANDLE */
};
