# Thermaltake X1 RGB (264A:3013) lighting protocol tester.
# Protocol reverse-engineered from "X1 RGB.exe" v1.10.0:
#   interface MI_01 (UsagePage 0xFF00), 64-byte output reports, report ID 0
#   C0 01 NN 00 [led r g b] x NN (NN <= 15)   -> set per-key colors
#   C0 F0 00 ...                              -> flush / latch frame
param(
    [ValidateSet('fill','led','sweep','off','full','raw','rows','init','extras')] [string]$Cmd = 'fill',
    [string]$Hex = '',
    [int]$Seconds = 0,
    [int]$R = 255, [int]$G = 0, [int]$B = 0,
    [int]$Led = -1,
    [int]$DelayMs = 400
)

$src = @"
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Collections.Generic;

public static class X1Hid {
    [StructLayout(LayoutKind.Sequential)]
    struct SP_DEVICE_INTERFACE_DATA { public int cbSize; public Guid g; public int Flags; public IntPtr Reserved; }
    [DllImport("hid.dll")] static extern void HidD_GetHidGuid(out Guid g);
    [DllImport("setupapi.dll", SetLastError = true)] static extern IntPtr SetupDiGetClassDevs(ref Guid g, IntPtr e, IntPtr h, int f);
    [DllImport("setupapi.dll", SetLastError = true)] static extern bool SetupDiEnumDeviceInterfaces(IntPtr s, IntPtr d, ref Guid g, int i, ref SP_DEVICE_INTERFACE_DATA did);
    [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)] static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr s, ref SP_DEVICE_INTERFACE_DATA did, IntPtr detail, int size, out int req, IntPtr di);
    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr s);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)] static extern SafeFileHandle CreateFile(string n, uint a, uint s, IntPtr sa, uint c, uint f, IntPtr t);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool WriteFile(SafeFileHandle h, byte[] b, int n, out int w, IntPtr o);

    public static string FindPath() {
        Guid g; HidD_GetHidGuid(out g);
        IntPtr set = SetupDiGetClassDevs(ref g, IntPtr.Zero, IntPtr.Zero, 0x12);
        var did = new SP_DEVICE_INTERFACE_DATA(); did.cbSize = Marshal.SizeOf(did);
        try {
            for (int i = 0; SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref g, i, ref did); i++) {
                int req; SetupDiGetDeviceInterfaceDetail(set, ref did, IntPtr.Zero, 0, out req, IntPtr.Zero);
                IntPtr buf = Marshal.AllocHGlobal(req);
                try {
                    Marshal.WriteInt32(buf, IntPtr.Size == 8 ? 8 : 6);
                    if (SetupDiGetDeviceInterfaceDetail(set, ref did, buf, req, out req, IntPtr.Zero)) {
                        string p = Marshal.PtrToStringAuto(new IntPtr(buf.ToInt64() + 4));
                        if (p.ToLower().Contains("vid_264a&pid_3013&mi_01")) return p;
                    }
                } finally { Marshal.FreeHGlobal(buf); }
            }
        } finally { SetupDiDestroyDeviceInfoList(set); }
        return null;
    }

    static SafeFileHandle h;
    public static void Open() {
        string p = FindPath();
        if (p == null) throw new Exception("X1 RGB control interface (264A:3013 MI_01) not found");
        h = CreateFile(p, 0xC0000000, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new Exception("CreateFile failed: " + Marshal.GetLastWin32Error());
    }
    public static void Close() { if (h != null) h.Dispose(); }

    // payload = 64 bytes; HID report = report ID 0 + payload
    public static void Send(byte[] payload) {
        byte[] rep = new byte[65];
        Array.Copy(payload, 0, rep, 1, Math.Min(64, payload.Length));
        int w;
        if (!WriteFile(h, rep, rep.Length, out w, IntPtr.Zero)) throw new Exception("WriteFile failed: " + Marshal.GetLastWin32Error());
    }

    public static void SetLeds(IList<int[]> leds) { // each: {led, r, g, b}
        for (int off = 0; off < leds.Count; off += 15) {
            int n = Math.Min(15, leds.Count - off);
            byte[] p = new byte[64];
            p[0] = 0xC0; p[1] = 0x01; p[2] = (byte)n;
            for (int i = 0; i < n; i++) {
                int[] e = leds[off + i];
                p[4 + i * 4] = (byte)e[0]; p[5 + i * 4] = (byte)e[1]; p[6 + i * 4] = (byte)e[2]; p[7 + i * 4] = (byte)e[3];
            }
            Send(p);
        }
        byte[] f = new byte[64]; f[0] = 0xC0; f[1] = 0xF0; Send(f);
    }
}
"@
Add-Type -TypeDefinition $src

# LED indices for US/ANSI layout (layout 0), in UI order, from FUN_0045e670.
$ledsUs = @(
 0x60,0x61,0x62,0x63,0x68,0x69,0x6a,0x70,0x71,0x72,0x43,0x44,0x45,
 0x00,0x01,0x08,0x09,0x10,0x11,0x18,0x19,0x20,0x21,0x28,0x29,0x30,0x31,
 0x02,0x03,0x0a,0x0b,0x12,0x13,0x1a,0x1b,0x22,0x23,0x2a,0x2b,0x32,0x33,
 0x04,0x05,0x0c,0x0d,0x14,0x15,0x1c,0x1d,0x24,0x25,0x2c,0x2d,0x34,
 0x06,0x07,0x0e,0x0f,0x16,0x17,0x1e,0x1f,0x26,0x27,0x2e,0x2f,
 0x5b,0x5a,0x5c,0x5d,0x5e,0x5f,0x3c,0x36,
 0x66,0x67,0x6b, 0x38,0x39,0x40, 0x3a,0x3b,0x42, 0x3d, 0x3f,0x3e,0x46,
 0x48,0x49,0x50,0x51, 0x4a,0x4b,0x52,0x53, 0x4c,0x4d,0x54, 0x4e,0x4f,0x56,0x55, 0x47,0x57,
 0x6d,0x6e,0x74,0x7d,0x75,0x7e
)

[X1Hid]::Open()
try {
    switch ($Cmd) {
        'fill' {
            $l = New-Object 'System.Collections.Generic.List[int[]]'
            foreach ($i in $ledsUs) { $l.Add([int[]]@($i, $R, $G, $B)) }
            [X1Hid]::SetLeds($l); "filled $($ledsUs.Count) LEDs with $R,$G,$B"
        }
        'full' {
            # set_led_full: C0 00 00 00 R G B, then flush C0 F0
            $p = New-Object byte[] 64; $p[0] = 0xC0; $p[4] = $R; $p[5] = $G; $p[6] = $B
            [X1Hid]::Send($p)
            $f = New-Object byte[] 64; $f[0] = 0xC0; $f[1] = 0xF0; [X1Hid]::Send($f)
            "full color $R,$G,$B sent"
        }
        'raw' {
            # send one arbitrary 64-byte payload given as hex, e.g. -Hex "C0F0"
            $bytes = [byte[]]($Hex -replace '\s','' -split '(..)' | Where-Object { $_ } | ForEach-Object { [Convert]::ToByte($_, 16) })
            $p = New-Object byte[] 64; [Array]::Copy($bytes, $p, [Math]::Min(64, $bytes.Length))
            [X1Hid]::Send($p); "raw sent: $Hex"
        }
        'init' {
            # host-control mode, sent first by the vendor app on connect (test_data())
            $p = New-Object byte[] 64; $p[0] = 0x41; $p[1] = 0x03; [X1Hid]::Send($p); "init 41 03 sent"
        }
        'rows' {
            # per-key (C0 01) test: each group of keys gets its own colour
            $groups = @(
                @{ n = 13; c = @(255,0,0) },     # Esc + F1..F12        red
                @{ n = 14; c = @(255,128,0) },   # number row           orange
                @{ n = 14; c = @(255,255,0) },   # Tab row              yellow
                @{ n = 13; c = @(0,255,0) },     # Caps row             green
                @{ n = 12; c = @(0,255,255) },   # Shift row            cyan
                @{ n = 8;  c = @(0,0,255) },     # bottom row           blue
                @{ n = 13; c = @(255,0,255) },   # PrtSc..arrows        magenta
                @{ n = 17; c = @(255,255,255) }, # numpad               white
                @{ n = 6;  c = @(128,0,255) }    # 6 extra LEDs         violet
            )
            $l = New-Object 'System.Collections.Generic.List[int[]]'
            $k = 0
            foreach ($grp in $groups) { for ($i = 0; $i -lt $grp.n; $i++) { $l.Add([int[]]@($ledsUs[$k], $grp.c[0], $grp.c[1], $grp.c[2])); $k++ } }
            [X1Hid]::SetLeds($l); "rows pattern sent ($k LEDs)"
        }
        'extras' {
            # the 6 non-keyboard-matrix LEDs, each a distinct colour; everything else off
            $ex = @{ 0x6d = @(255,0,0); 0x6e = @(0,255,0); 0x74 = @(0,0,255); 0x7d = @(255,255,0); 0x75 = @(0,255,255); 0x7e = @(255,255,255) }
            $l = New-Object 'System.Collections.Generic.List[int[]]'
            for ($i = 0; $i -lt 128; $i++) {
                if ($ex.ContainsKey($i)) { $c = $ex[$i]; $l.Add([int[]]@($i, $c[0], $c[1], $c[2])) } else { $l.Add([int[]]@($i, 0, 0, 0)) }
            }
            # with -Seconds N keep re-sending (e.g. while the OpenRGB plugin refreshes its own frame)
            $until = (Get-Date).AddSeconds($Seconds)
            do { [X1Hid]::SetLeds($l); if ($Seconds -gt 0) { Start-Sleep -Milliseconds 300 } } while ((Get-Date) -lt $until)
            "extras: 6D red, 6E green, 74 blue, 7D yellow, 75 cyan, 7E white"
        }
        'off' {
            $l = New-Object 'System.Collections.Generic.List[int[]]'
            for ($i = 0; $i -lt 128; $i++) { $l.Add([int[]]@($i, 0, 0, 0)) }
            [X1Hid]::SetLeds($l); "all 128 LED slots off"
        }
        'led' {
            $l = New-Object 'System.Collections.Generic.List[int[]]'
            for ($i = 0; $i -lt 128; $i++) { if ($i -eq $Led) { $l.Add([int[]]@($i, $R, $G, $B)) } else { $l.Add([int[]]@($i, 0, 0, 0)) } }
            [X1Hid]::SetLeds($l); "only LED 0x{0:X2} lit" -f $Led
        }
        'sweep' {
            foreach ($i in $ledsUs) {
                $l = New-Object 'System.Collections.Generic.List[int[]]'
                foreach ($j in $ledsUs) { if ($j -eq $i) { $l.Add([int[]]@($j, $R, $G, $B)) } else { $l.Add([int[]]@($j, 0, 0, 0)) } }
                [X1Hid]::SetLeds($l); Start-Sleep -Milliseconds $DelayMs
            }
            "sweep done"
        }
    }
} finally { [X1Hid]::Close() }
