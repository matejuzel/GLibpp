# Runtime gate pro GLibpp: spusti exe, zmeri frame pacing z titulku okna,
# volitelne simuluje jizdu, ulozi screenshot a otestuje ESC shutdown.
# Samostatny skript - nese si vlastni Add-Type (stav shellu mezi volanimi nepersistuje).
param(
    [ValidateSet("Debug", "Release")] [string]$Config = "Debug",
    [switch]$Drive,
    [switch]$TestEsc,
    [string]$Screenshot = "",
    [int]$MinLow = 52,
    [int]$Samples = 5,
    [int]$WarmupSeconds = 6
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class GLibVerify {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
'@

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$exe = Join-Path $repoRoot "x64\$Config\GLibpp.exe"
if (-not (Test-Path $exe)) { Write-Output "FAIL: $exe neexistuje - nejdriv build"; exit 1 }

# uklid pripadne bezici instance
try { Stop-Process -Name GLibpp -Force -ErrorAction Stop } catch {}

Start-Process -FilePath $exe -WorkingDirectory $repoRoot
Start-Sleep -Seconds $WarmupSeconds
$p = Get-Process GLibpp -ErrorAction SilentlyContinue
if (-not $p) { Write-Output "FAIL: proces po startu nezije"; exit 1 }

if ($Drive) {
    [GLibVerify]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 500
    [GLibVerify]::keybd_event(0x26, 0, 0, [UIntPtr]::Zero)   # UP down
    Start-Sleep -Seconds 2
    [GLibVerify]::keybd_event(0x25, 0, 0, [UIntPtr]::Zero)   # LEFT down
    Start-Sleep -Seconds 1
    [GLibVerify]::keybd_event(0x25, 0, 2, [UIntPtr]::Zero)   # LEFT up
    [GLibVerify]::keybd_event(0x26, 0, 2, [UIntPtr]::Zero)   # UP up
    Start-Sleep -Milliseconds 300
}

if ($Screenshot -ne "") {
    $r = New-Object "GLibVerify+RECT"
    [GLibVerify]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
    $w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
    $bmp.Save($Screenshot, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Output "screenshot: $Screenshot ($w x $h)"
}

# mereni: titulek nese "FPS: n ||| 1% Low: n ||| ..." aktualizovany 1x/s render vlaknem
$titles = @()
$lows = @()
for ($i = 0; $i -lt $Samples; $i++) {
    $proc = Get-Process GLibpp -ErrorAction SilentlyContinue
    if (-not $proc) { Write-Output "FAIL: proces behem mereni umrel"; exit 1 }
    $t = $proc.MainWindowTitle
    if ($t -match '1% Low: (\d+)') {
        $titles += $t
        $lows += [int]$Matches[1]
    }
    Start-Sleep -Seconds 1
}

$titles | ForEach-Object { Write-Output $_ }

if ($TestEsc) {
    [GLibVerify]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 300
    [GLibVerify]::keybd_event(0x1B, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [GLibVerify]::keybd_event(0x1B, 0, 2, [UIntPtr]::Zero)
    Start-Sleep -Seconds 2
    if (Get-Process GLibpp -ErrorAction SilentlyContinue) {
        Write-Output "FAIL: ESC shutdown nefunguje"
        Stop-Process -Name GLibpp -Force
        exit 1
    }
    Write-Output "ESC shutdown OK"
} else {
    try { Stop-Process -Name GLibpp -Force -ErrorAction Stop } catch {}
}

if ($lows.Count -eq 0) { Write-Output "FAIL: nenamereno nic (titulek bez FPS stats)"; exit 1 }
$minLowMeasured = ($lows | Measure-Object -Minimum).Minimum
if ($minLowMeasured -lt $MinLow) {
    Write-Output "FAIL: min 1% Low = $minLowMeasured < $MinLow"
    exit 1
}
Write-Output "PASS: min 1% Low = $minLowMeasured (prah $MinLow)"
