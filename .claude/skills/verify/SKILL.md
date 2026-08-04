---
name: verify
description: Build GLibpp (x64) and verify runtime health - run the exe, gate on frame pacing (FPS / 1% Low from the window title), optionally simulate driving (arrow keys), capture a screenshot and test ESC shutdown. Use after any code change touching the renderer, timing, Slerp, Scene or per-frame work.
---

# Verifikace buildu a runtime (GLibpp)

Standardní gate po každé změně kódu. Dva kroky: build, pak měřicí skript.

## 1. Build

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" GLibpp.sln -p:Configuration=Debug -p:Platform=x64 -v:minimal -m
```

Pro Release zaměň `Debug` za `Release`. Diagnostika chodí česky (českolokalizovaný MSVC). Pokud build selže, neopravuj přepnutím platformy — jen x64 je funkční.

## 2. Runtime gate

Spusť přibalený skript (samostatný, nese si vlastní Add-Type — stav shellu mezi voláními nepersistuje):

```powershell
& ".claude\skills\verify\verify.ps1" -Config Debug -Samples 5
```

Parametry:
- `-Config Debug|Release` — který build spustit (default Debug)
- `-Drive` — před měřením podrží šipku nahoru 2 s + doleva 1 s (ověří fyziku/vstup; okno dostane focus)
- `-Screenshot <cesta.png>` — uloží screenshot okna (pak ho přečti nástrojem Read a zkontroluj vizuál: vlnící se grid, auto, 4 kola, wireframe koule, barevné osy)
- `-TestEsc` — na konci pošle ESC a ověří, že proces korektně skončil (App joinuje render vlákno)
- `-MinLow <n>` — práh pro 1% Low (default 52)
- `-Samples <n>` — počet vzorků titulku po 1 s (default 5)

## 3. Interpretace

- **Zdravé:** FPS 60, 1% Low ≥ 52. Skript končí exit 0 / `PASS`.
- **1% Low ~31** = vynechané vblanky (33ms framy) — regrese pacingu.
- V konzoli aplikace hlídej řádky `Zaskub: t = ...` — skutečné zaseknutí logiky (interpolační alfa přeteklo).
- Před/po měření srovnávej na stejném stroji (baseline: laptop Ryzen 7 5825U).
