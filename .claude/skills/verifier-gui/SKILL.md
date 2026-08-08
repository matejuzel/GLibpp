---
name: verifier-gui
description: GUI evidence-capture verifier for GLibpp - build (x64), run the exe, gate on frame pacing (FPS / 1% Low from the window title), simulate driving (arrow keys), capture screenshots and test ESC shutdown. Use to verify any change at the app's window surface (renderer, timing, Slerp, Scene, per-frame work).
---

# Verifikace na GUI povrchu (GLibpp)

Standardní gate po každé změně kódu. Dva kroky: build, pak měřicí skript.

## 1. Build

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" GLibpp.sln -p:Configuration=Debug -p:Platform=x64 -v:minimal -m
```

Pro Release zaměň `Debug` za `Release`. Diagnostika chodí česky (českolokalizovaný MSVC). Pokud build selže, neopravuj přepnutím platformy — jen x64 je funkční.

## 2. Runtime gate

Spusť přibalený skript (samostatný, nese si vlastní Add-Type — stav shellu mezi voláními nepersistuje):

```powershell
& ".claude\skills\verifier-gui\verify.ps1" -Config Debug -Samples 5
```

Parametry:
- `-Config Debug|Release` — který build spustit (default Debug)
- `-Drive` — před měřením podrží šipku nahoru 2 s + doleva 1 s (ověří fyziku/vstup; okno dostane focus)
- `-Screenshot <cesta.png>` — uloží screenshot okna (pak ho přečti nástrojem Read a zkontroluj vizuál: vlnící se grid, auto, 4 kola, wireframe koule, barevné osy)
- `-TestEsc` — na konci pošle ESC a ověří, že proces korektně skončil (App joinuje render vlákno)
- `-MinLow <n>` — práh pro 1% Low (default 52)
- `-Samples <n>` — počet vzorků titulku po 1 s (default 5)

### Omezení `-Drive` a `-Screenshot`

Injektáž kláves (`keybd_event`) a screenshot (`CopyFromScreen`) fungují jen když okno GLibpp reálně získá popředí. **Pokud uživatel u stroje aktivně pracuje**, Windows foreground-lock `SetForegroundWindow` z pozadí odmítne — klávesy pak padají do uživatelova okna a screenshot vyfotí cizí obsah (snímá se oblast obrazovky, ne okno). V takovém případě se spolehni jen na titulkové vzorky a stdout (ty jdou přímo z procesu) a vizuální ověření odlož, nebo požádej uživatele. Stdout aplikace lze zachytit přes `Start-Process -RedirectStandardOutput` (handly bufferů, `resized:` eventy, `Zaskub` řádky).

## 3. Interpretace

Zdravé hodnoty závisí na stroji — DwmFlush pacuje framy na refresh rate monitoru. Před/po měření srovnávej vždy na tomtéž stroji, proti odpovídající baseline:

| Stroj | Zdravé | Regrese pacingu | Práh |
|---|---|---|---|
| **Laptop** — Ryzen 7 5825U, 60Hz displej | FPS 60, 1% Low ≥ 52 | 1% Low ~31 = vynechané vblanky (33ms framy) | default `-MinLow 52` |
| **Desktop** — Ryzen 7 9700X, 64GB DDR5, RTX 5070 Ti 16GB, 2560×1080 @ 199 Hz | FPS ~190–200, 1% Low ~95–105 (Debug, změřeno 2026-08-08) | 1% Low výrazně pod ~90 = vynechané kompozice | předávej `-MinLow 90` |

- Default `-MinLow 52` ve skriptu je kalibrovaný na laptop — na desktopu by prošla i reálná regrese, proto tam práh explicitně zvyš.
- V konzoli aplikace hlídej řádky `Zaskub: t = ...` — skutečné zaseknutí logiky (interpolační alfa přeteklo).
- Skript končí exit 0 / `PASS` při splnění prahu.
