/*---------------------------------------------------------*\
| X1RGBDevice.cpp                                           |
|                                                           |
|   Windows HID transport for the Thermaltake X1 RGB.       |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "X1RGBDevice.h"

#include <algorithm>
#include <cstring>
#include <cwctype>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#else
#error "X1RGBDevice currently implements the Windows HID transport only"
#endif

static const DWORD X1RGB_WRITE_TIMEOUT_MS = 250;

static std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
    return s;
}

/*---------------------------------------------------------*\
| Check that the interface is the vendor lighting one       |
| (usage page 0xFF00, 64-byte output reports).              |
\*---------------------------------------------------------*/
static bool IsLightingInterface(const std::wstring& path)
{
    HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(h == INVALID_HANDLE_VALUE)
    {
        return(false);
    }

    bool                    ok = false;
    PHIDP_PREPARSED_DATA    pp = NULL;

    if(HidD_GetPreparsedData(h, &pp))
    {
        HIDP_CAPS caps;

        if(HidP_GetCaps(pp, &caps) == HIDP_STATUS_SUCCESS)
        {
            ok = (caps.UsagePage == 0xFF00) && (caps.OutputReportByteLength == (X1RGB_PACKET_SIZE + 1));
        }

        HidD_FreePreparsedData(pp);
    }

    CloseHandle(h);
    return(ok);
}

std::vector<std::wstring> X1RGBDevice::Enumerate()
{
    std::vector<std::wstring> result;

    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if(set == INVALID_HANDLE_VALUE)
    {
        return(result);
    }

    SP_DEVICE_INTERFACE_DATA ifdata;
    ifdata.cbSize = sizeof(ifdata);

    for(DWORD idx = 0; SetupDiEnumDeviceInterfaces(set, NULL, &hid_guid, idx, &ifdata); idx++)
    {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(set, &ifdata, NULL, 0, &required, NULL);

        if(required == 0)
        {
            continue;
        }

        std::vector<unsigned char> buf(required);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buf.data();
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if(!SetupDiGetDeviceInterfaceDetailW(set, &ifdata, detail, required, NULL, NULL))
        {
            continue;
        }

        std::wstring path  = detail->DevicePath;
        std::wstring lower = ToLower(path);

        if((lower.find(L"vid_264a&pid_3013") != std::wstring::npos)
        && (lower.find(L"mi_01") != std::wstring::npos)
        && IsLightingInterface(path))
        {
            result.push_back(path);
        }
    }

    SetupDiDestroyDeviceInfoList(set);
    return(result);
}

X1RGBDevice::X1RGBDevice(const std::wstring& path) : path(path), handle(INVALID_HANDLE_VALUE), write_event(NULL)
{
    write_event = CreateEventW(NULL, TRUE, FALSE, NULL);
}

X1RGBDevice::~X1RGBDevice()
{
    Close();

    if(write_event)
    {
        CloseHandle((HANDLE)write_event);
        write_event = NULL;
    }
}

bool X1RGBDevice::Open()
{
    Close();

    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if(h == INVALID_HANDLE_VALUE)
    {
        return(false);
    }

    handle = h;

    if(!EnterHostMode())
    {
        Close();
        return(false);
    }

    return(true);
}

void X1RGBDevice::Close()
{
    if(handle != INVALID_HANDLE_VALUE)
    {
        CancelIo((HANDLE)handle);
        CloseHandle((HANDLE)handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

bool X1RGBDevice::IsOpen() const
{
    return(handle != INVALID_HANDLE_VALUE);
}

std::string X1RGBDevice::GetPathUTF8() const
{
    int len = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0, NULL, NULL);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), &out[0], len, NULL, NULL);
    return(out);
}

/*---------------------------------------------------------*\
| Send one 64-byte payload as HID output report 0           |
\*---------------------------------------------------------*/
bool X1RGBDevice::Write(const uint8_t payload[X1RGB_PACKET_SIZE])
{
    if(handle == INVALID_HANDLE_VALUE || write_event == NULL)
    {
        return(false);
    }

    uint8_t report[X1RGB_PACKET_SIZE + 1];
    report[0] = 0x00;
    memcpy(&report[1], payload, X1RGB_PACKET_SIZE);

    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = (HANDLE)write_event;
    ResetEvent(ov.hEvent);

    DWORD written = 0;

    if(!WriteFile((HANDLE)handle, report, sizeof(report), &written, &ov))
    {
        if(GetLastError() != ERROR_IO_PENDING)
        {
            return(false);
        }

        if(WaitForSingleObject(ov.hEvent, X1RGB_WRITE_TIMEOUT_MS) != WAIT_OBJECT_0)
        {
            CancelIo((HANDLE)handle);
            GetOverlappedResult((HANDLE)handle, &ov, &written, TRUE);
            return(false);
        }

        if(!GetOverlappedResult((HANDLE)handle, &ov, &written, FALSE))
        {
            return(false);
        }
    }

    return(written == sizeof(report));
}

bool X1RGBDevice::EnterHostMode()
{
    uint8_t p[X1RGB_PACKET_SIZE] = { 0 };
    p[0] = 0x41;
    p[1] = 0x03;
    return(Write(p));
}

bool X1RGBDevice::Latch()
{
    uint8_t p[X1RGB_PACKET_SIZE] = { 0 };
    p[0] = 0xC0;
    p[1] = 0xF0;
    return(Write(p));
}

bool X1RGBDevice::SendFrame(const std::vector<X1LedColor>& leds)
{
    for(std::size_t offset = 0; offset < leds.size(); offset += X1RGB_LEDS_PER_PACKET)
    {
        std::size_t count = std::min<std::size_t>(X1RGB_LEDS_PER_PACKET, leds.size() - offset);

        uint8_t p[X1RGB_PACKET_SIZE] = { 0 };
        p[0] = 0xC0;
        p[1] = 0x01;
        p[2] = (uint8_t)count;

        for(std::size_t i = 0; i < count; i++)
        {
            const X1LedColor& c = leds[offset + i];
            p[4 + (i * 4) + 0]  = c.led;
            p[4 + (i * 4) + 1]  = c.r;
            p[4 + (i * 4) + 2]  = c.g;
            p[4 + (i * 4) + 3]  = c.b;
        }

        if(!Write(p))
        {
            return(false);
        }
    }

    return(Latch());
}

bool X1RGBDevice::SendFullColor(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t p[X1RGB_PACKET_SIZE] = { 0 };
    p[0] = 0xC0;
    p[4] = r;
    p[5] = g;
    p[6] = b;
    return(Write(p) && Latch());
}
