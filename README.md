# OpenRGB plugin: Thermaltake Premium X1 RGB

Adds the Thermaltake (Tt eSPORTS) Premium X1 RGB keyboard (USB `264A:3013`) to
OpenRGB 1.1 as a per-key **Direct** device. Stock OpenRGB 1.1 has no driver for it.

- OpenRGB: 1.1 (`release_1.1`, commit `8e16de27858c`), plugin API 5
- Qt 6.8.3 (msvc2022_64), MSVC x64, Windows only (native HID transport)
- Zones: `Keyboard` (6×22 matrix, 104 keys ANSI / 105 ISO) and `Media Keys` (Stop, Previous, Play/Pause, Next)

## Install

Download `ThermaltakeX1RGBPlugin.dll` from
[Releases](https://github.com/polosatiktiger/openrgb-thermaltake-x1-rgb-plugin/releases)
(or build it) and copy it to `%APPDATA%\OpenRGB\plugins\` (per user) or
`C:\Program Files\OpenRGB\plugins\` (all users), then start the OpenRGB GUI.

The headless OpenRGB service does not load plugins, so the GUI must be running for
the keyboard to be controlled. It connects to the service as a client.

The plugin adds an info tab, *Information → Thermaltake X1 RGB*, that shows the
connection status.

Layout defaults to ANSI. For ISO keyboards, add this to `OpenRGB.json`:

```json
"ThermaltakeX1RGBPlugin": { "layout": "ISO" }
```

## Build

Requirements:

- VS 2022 Build Tools with the C++ workload
- Qt 6.8.3 msvc2022_64 (only `qtbase` is needed)
- OpenRGB `release_1.1` sources, used for the plugin API headers

```bat
git clone --depth 1 --branch release_1.1 https://gitlab.com/CalcProgrammer1/OpenRGB.git OpenRGB
build.bat
```

`build.bat` looks for Qt in `C:\Qt\6.8.3\msvc2022_64`, for OpenRGB in `.\OpenRGB`,
and finds Visual Studio with `vswhere`. To use other locations, set `QTDIR`,
`OPENRGB_SRC` or `VCVARS`, either in the environment or in a `build.local.bat` next
to `build.bat`. That file is git-ignored. The result is `build\ThermaltakeX1RGBPlugin.dll`.

The plugin must be built against the same OpenRGB plugin API (5) and Qt major
version as the OpenRGB it is loaded into.

## Protocol tester

`tools\x1test.ps1` talks to the keyboard directly, without OpenRGB, and is useful for
protocol experiments:

```powershell
.\tools\x1test.ps1 -Cmd init                    # 41 03, host-control mode
.\tools\x1test.ps1 -Cmd full -R 0 -G 255 -B 0   # whole keyboard green
.\tools\x1test.ps1 -Cmd rows                    # colour each key group differently
.\tools\x1test.ps1 -Cmd raw -Hex "C0F0"         # send an arbitrary payload
```

Close OpenRGB first, or the plugin will overwrite the tester's output within 5 s.

## Protocol

Reverse-engineered from `X1 RGB.exe` v1.10.0 (installer `X1_RGB_setup_V1.10.0.exe`).
Everything below was confirmed on hardware unless marked otherwise.

| Item | Value |
|---|---|
| USB | VID `0x264A`, PID `0x3013`; `0x3012` = firmware-update (bootloader) mode |
| Lighting interface | interface 1 (`MI_01`), HID usage page `0xFF00`, 64-byte reports, no report IDs |
| Endpoints (vendor app uses libusb) | OUT `0x04`, IN `0x83`, interrupt |
| On Windows | `WriteFile` of 65 bytes: `00` (report ID) + 64-byte payload |

Payloads are 64 bytes, zero-padded:

| Bytes | Meaning |
|---|---|
| `41 03` | Enter host-control mode. **Required**: without it lighting commands are ignored. |
| `C0 01 NN 00` + NN × `[LED R G B]` | Set up to 15 LEDs per packet. |
| `C0 00 00 00 R G B` | Set all LEDs to one colour. |
| `C0 F0` | Latch: show the colours sent since the last latch. |
| `51 00 00 00 P` / `50` | Select / read onboard profile (seen in vendor app, not used). |
| `12 22` | Read firmware version; the reply starts `12 22` and the version is in byte 8 (not used). |

The firmware has no host-selectable effects. The vendor app renders every effect
(wave, ripple, spectrum…) in software and streams frames as `C0 01…` + `C0 F0`.

Colours are sent unscaled. The vendor app applies brightness itself as
`c × level × 25 / 100`, where level is 0–4.

### LED indices

Columns are laid out in pairs of 8: `index = (column/2)*8 + row_offset + column%2`.
The full ANSI and ISO tables are in `X1RGBLayout.h`.

| Group | LED indices |
|---|---|
| F row | Esc `60`, F1–F3 `61–63`, F4–F6 `68–6A`, F7–F9 `70–72`, F10–F12 `43–45` |
| Number row | `00 01 08 09 10 11 18 19 20 21 28 29 30 31` |
| Bottom row | LCtrl `5B`, LWin `5A`, LAlt `5C`, Space `5D`, RAlt `5E`, Fn `5F`, Menu `3C`, RCtrl `36` |
| ISO extras | `#` `6F`, `\` `35`; the ANSI `\` (`33`) is absent |
| Media keys | Stop `74`, Previous `7D`, Play/Pause `75`, Next `7E`. `6D`/`6E` are addressed by the vendor app but not populated on this unit. The Mute key backlight is not addressable (it stays lit with every index set to black). |

## Limitations

- Windows only. A Linux port needs a hidapi/hidraw transport in `X1RGBDevice.cpp`.
- ISO layout is decoded from the vendor app but not hardware-tested. JP/KR layouts
  (vendor layouts 2 and 3) are not implemented.
- The device is found only when the plugin loads. After plugging the keyboard in
  later, restart OpenRGB. Unplug/replug and sleep while the plugin is running are
  handled: the last frame is resent every 5 s.
- The command that returns the keyboard to its onboard lighting is unknown. After
  OpenRGB closes the keyboard keeps the last colours until it is replugged.

## License

GPL-2.0-or-later, same as OpenRGB, whose plugin API headers this plugin is built
against. See `LICENSE`.

Thermaltake and Tt eSPORTS are trademarks of their owners. This project is not
affiliated with or endorsed by Thermaltake.
