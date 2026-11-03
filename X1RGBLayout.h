/*---------------------------------------------------------*\
| X1RGBLayout.h                                             |
|                                                           |
|   Key/LED tables for the Thermaltake Premium X1 RGB.      |
|                                                           |
|   LED values were recovered from the vendor software      |
|   (X1 RGB.exe v1.10.0, functions FUN_0045e670 = layout 0  |
|   "US/ANSI" and FUN_0045fd70 = layout 1 "ISO") and        |
|   verified on real hardware (ANSI).                       |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <cstdint>

struct X1Key
{
    const char*     name;       /* OpenRGB key name (RGBControllerKeyNames.cpp) */
    uint8_t         value;      /* LED index used by the keyboard firmware      */
    uint8_t         row;        /* Matrix row                                   */
    uint8_t         col;        /* Matrix column                                */
};

struct X1Layout
{
    const char*     name;
    const X1Key*    keys;
    unsigned int    key_count;
};

static const unsigned int X1_MATRIX_HEIGHT = 6;
static const unsigned int X1_MATRIX_WIDTH  = 22;

/*---------------------------------------------------------*\
| Keys shared by both layouts, in vendor UI order.          |
| Order only affects OpenRGB LED numbering; the firmware    |
| addresses LEDs by value.                                  |
\*---------------------------------------------------------*/
#define X1_KEYS_FUNCTION_ROW                                        \
    { "Key: Escape",            0x60, 0,  0 },                      \
    { "Key: F1",                0x61, 0,  2 },                      \
    { "Key: F2",                0x62, 0,  3 },                      \
    { "Key: F3",                0x63, 0,  4 },                      \
    { "Key: F4",                0x68, 0,  5 },                      \
    { "Key: F5",                0x69, 0,  6 },                      \
    { "Key: F6",                0x6A, 0,  7 },                      \
    { "Key: F7",                0x70, 0,  8 },                      \
    { "Key: F8",                0x71, 0,  9 },                      \
    { "Key: F9",                0x72, 0, 10 },                      \
    { "Key: F10",               0x43, 0, 11 },                      \
    { "Key: F11",               0x44, 0, 12 },                      \
    { "Key: F12",               0x45, 0, 13 }

#define X1_KEYS_NUMBER_ROW                                          \
    { "Key: `",                 0x00, 1,  0 },                      \
    { "Key: 1",                 0x01, 1,  1 },                      \
    { "Key: 2",                 0x08, 1,  2 },                      \
    { "Key: 3",                 0x09, 1,  3 },                      \
    { "Key: 4",                 0x10, 1,  4 },                      \
    { "Key: 5",                 0x11, 1,  5 },                      \
    { "Key: 6",                 0x18, 1,  6 },                      \
    { "Key: 7",                 0x19, 1,  7 },                      \
    { "Key: 8",                 0x20, 1,  8 },                      \
    { "Key: 9",                 0x21, 1,  9 },                      \
    { "Key: 0",                 0x28, 1, 10 },                      \
    { "Key: -",                 0x29, 1, 11 },                      \
    { "Key: =",                 0x30, 1, 12 },                      \
    { "Key: Backspace",         0x31, 1, 13 }

#define X1_KEYS_TAB_ROW_COMMON                                      \
    { "Key: Tab",               0x02, 2,  0 },                      \
    { "Key: Q",                 0x03, 2,  1 },                      \
    { "Key: W",                 0x0A, 2,  2 },                      \
    { "Key: E",                 0x0B, 2,  3 },                      \
    { "Key: R",                 0x12, 2,  4 },                      \
    { "Key: T",                 0x13, 2,  5 },                      \
    { "Key: Y",                 0x1A, 2,  6 },                      \
    { "Key: U",                 0x1B, 2,  7 },                      \
    { "Key: I",                 0x22, 2,  8 },                      \
    { "Key: O",                 0x23, 2,  9 },                      \
    { "Key: P",                 0x2A, 2, 10 },                      \
    { "Key: [",                 0x2B, 2, 11 },                      \
    { "Key: ]",                 0x32, 2, 12 }

#define X1_KEYS_CAPS_ROW_COMMON                                     \
    { "Key: Caps Lock",         0x04, 3,  0 },                      \
    { "Key: A",                 0x05, 3,  1 },                      \
    { "Key: S",                 0x0C, 3,  2 },                      \
    { "Key: D",                 0x0D, 3,  3 },                      \
    { "Key: F",                 0x14, 3,  4 },                      \
    { "Key: G",                 0x15, 3,  5 },                      \
    { "Key: H",                 0x1C, 3,  6 },                      \
    { "Key: J",                 0x1D, 3,  7 },                      \
    { "Key: K",                 0x24, 3,  8 },                      \
    { "Key: L",                 0x25, 3,  9 },                      \
    { "Key: ;",                 0x2C, 3, 10 },                      \
    { "Key: '",                 0x2D, 3, 11 }

#define X1_KEYS_SHIFT_ROW_LETTERS                                   \
    { "Key: Z",                 0x07, 4,  2 },                      \
    { "Key: X",                 0x0E, 4,  3 },                      \
    { "Key: C",                 0x0F, 4,  4 },                      \
    { "Key: V",                 0x16, 4,  5 },                      \
    { "Key: B",                 0x17, 4,  6 },                      \
    { "Key: N",                 0x1E, 4,  7 },                      \
    { "Key: M",                 0x1F, 4,  8 },                      \
    { "Key: ,",                 0x26, 4,  9 },                      \
    { "Key: .",                 0x27, 4, 10 },                      \
    { "Key: /",                 0x2E, 4, 11 },                      \
    { "Key: Right Shift",       0x2F, 4, 13 }

#define X1_KEYS_BOTTOM_ROW                                          \
    { "Key: Left Control",      0x5B, 5,  0 },                      \
    { "Key: Left Windows",      0x5A, 5,  1 },                      \
    { "Key: Left Alt",          0x5C, 5,  2 },                      \
    { "Key: Space",             0x5D, 5,  6 },                      \
    { "Key: Right Alt",         0x5E, 5, 10 },                      \
    { "Key: Right Fn",          0x5F, 5, 11 },                      \
    { "Key: Menu",              0x3C, 5, 12 },                      \
    { "Key: Right Control",     0x36, 5, 13 }

#define X1_KEYS_NAVIGATION                                          \
    { "Key: Print Screen",      0x66, 0, 15 },                      \
    { "Key: Scroll Lock",       0x67, 0, 16 },                      \
    { "Key: Pause/Break",       0x6B, 0, 17 },                      \
    { "Key: Insert",            0x38, 1, 15 },                      \
    { "Key: Home",              0x39, 1, 16 },                      \
    { "Key: Page Up",           0x40, 1, 17 },                      \
    { "Key: Delete",            0x3A, 2, 15 },                      \
    { "Key: End",               0x3B, 2, 16 },                      \
    { "Key: Page Down",         0x42, 2, 17 },                      \
    { "Key: Up Arrow",          0x3D, 4, 16 },                      \
    { "Key: Left Arrow",        0x3F, 5, 15 },                      \
    { "Key: Down Arrow",        0x3E, 5, 16 },                      \
    { "Key: Right Arrow",       0x46, 5, 17 }

#define X1_KEYS_NUMPAD                                              \
    { "Key: Num Lock",          0x48, 1, 18 },                      \
    { "Key: Number Pad /",      0x49, 1, 19 },                      \
    { "Key: Number Pad *",      0x50, 1, 20 },                      \
    { "Key: Number Pad -",      0x51, 1, 21 },                      \
    { "Key: Number Pad 7",      0x4A, 2, 18 },                      \
    { "Key: Number Pad 8",      0x4B, 2, 19 },                      \
    { "Key: Number Pad 9",      0x52, 2, 20 },                      \
    { "Key: Number Pad +",      0x53, 2, 21 },                      \
    { "Key: Number Pad 4",      0x4C, 3, 18 },                      \
    { "Key: Number Pad 5",      0x4D, 3, 19 },                      \
    { "Key: Number Pad 6",      0x54, 3, 20 },                      \
    { "Key: Number Pad 1",      0x4E, 4, 18 },                      \
    { "Key: Number Pad 2",      0x4F, 4, 19 },                      \
    { "Key: Number Pad 3",      0x56, 4, 20 },                      \
    { "Key: Number Pad Enter",  0x55, 4, 21 },                      \
    { "Key: Number Pad 0",      0x47, 5, 18 },                      \
    { "Key: Number Pad .",      0x57, 5, 20 }

/*---------------------------------------------------------*\
| US / ANSI (vendor layout 0) - 104 keys, hardware-verified |
\*---------------------------------------------------------*/
static const X1Key x1_keys_ansi[] =
{
    X1_KEYS_FUNCTION_ROW,
    X1_KEYS_NUMBER_ROW,
    X1_KEYS_TAB_ROW_COMMON,
    { "Key: \\ (ANSI)",         0x33, 2, 13 },
    X1_KEYS_CAPS_ROW_COMMON,
    { "Key: Enter",             0x34, 3, 13 },
    { "Key: Left Shift",        0x06, 4,  0 },
    X1_KEYS_SHIFT_ROW_LETTERS,
    X1_KEYS_BOTTOM_ROW,
    X1_KEYS_NAVIGATION,
    X1_KEYS_NUMPAD,
};

/*---------------------------------------------------------*\
| ISO / UK-EU (vendor layout 1) - 105 keys                  |
| Differs from ANSI: no ANSI backslash (0x33), extra '#'    |
| key left of Enter (0x6F) and extra '\' key right of the   |
| left Shift (0x35). Not hardware-verified.                 |
\*---------------------------------------------------------*/
static const X1Key x1_keys_iso[] =
{
    X1_KEYS_FUNCTION_ROW,
    X1_KEYS_NUMBER_ROW,
    X1_KEYS_TAB_ROW_COMMON,
    X1_KEYS_CAPS_ROW_COMMON,
    { "Key: #",                 0x6F, 3, 12 },
    { "Key: Enter (ISO)",       0x34, 3, 13 },
    { "Key: Left Shift",        0x06, 4,  0 },
    { "Key: \\ (ISO)",          0x35, 4,  1 },
    X1_KEYS_SHIFT_ROW_LETTERS,
    X1_KEYS_BOTTOM_ROW,
    X1_KEYS_NAVIGATION,
    X1_KEYS_NUMPAD,
};

static const X1Layout x1_layout_ansi = { "ANSI", x1_keys_ansi, sizeof(x1_keys_ansi) / sizeof(x1_keys_ansi[0]) };
static const X1Layout x1_layout_iso  = { "ISO",  x1_keys_iso,  sizeof(x1_keys_iso)  / sizeof(x1_keys_iso[0])  };

/*---------------------------------------------------------*\
| Media keys outside the key matrix (hardware-verified).    |
| The vendor software also addresses 0x6D and 0x6E, but on  |
| the tested keyboard they are not connected. The Mute key  |
| backlight is not addressable by this protocol (it stays   |
| lit while every LED index 0x00-0x7F is set to black).     |
\*---------------------------------------------------------*/
static const X1Key x1_extra_leds[] =
{
    { "Key: Media Stop",        0x74, 0, 0 },
    { "Key: Media Previous",    0x7D, 0, 0 },
    { "Key: Media Play/Pause",  0x75, 0, 0 },
    { "Key: Media Next",        0x7E, 0, 0 },
};

static const unsigned int x1_extra_led_count = sizeof(x1_extra_leds) / sizeof(x1_extra_leds[0]);
