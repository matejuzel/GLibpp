---
name: glib-commit
description: Commit ritual for GLibpp - encoding check, build gate, Czech commit message in the "vyvoj - <co>" style. Use whenever committing changes in this repo.
---

# Commit rituál (GLibpp)

Pořadí je závazné — v historii repa už jednou mojibake i rozbitý stav commitnuté byly.

## 1. Kontrola kódování

```powershell
& ".claude\skills\glib-check-encoding\check-encoding.ps1"
```

Při nálezu **necommituj** — oprav podle instrukcí skillu `glib-check-encoding`.

## 2. Build gate

Debug|x64 musí projít (u větších změn i Release|x64):

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" GLibpp.sln -p:Configuration=Debug -p:Platform=x64 -v:minimal -m
```

U změn dotýkajících se rendereru/timingu navíc runtime gate — skill `verifier-gui`.

## 3. Testy

Solution staví i projekt `GLibppTests` — spusť ho a čekej exit 0:

```powershell
& ".\x64\Debug\GLibppTests.exe"
```

Při selhání **necommituj**. Testy sahají na skutečné hlavičky enginu (rasterizace, projekce, MeshFactory), takže selhání znamená regresi, ne rozbitý test.

## 4. Commit

- Zkontroluj `git status` + `git diff` — nikdy nestageuj `GLibpp/_old/` ani soubory, které do změny nepatří.
- Zpráva **česky**, subjekt ve stylu `vyvoj - <stručně co>` (viz `git log --oneline`); pro čistě úklidové commity je v historii i vzor `vyreseni warningu`, `uprava`.
- Commituje se přímo do `main` (pracovní konvence tohoto repa).
- `Co-Authored-By` uveď podle modelu, který na změně skutečně pracoval.

Víceřádkovou zprávu předávej **souborem**, ne here-stringem — `git add ...; git commit -m @'...'@` na jednom řádku se v PowerShellu už jednou rozsypal na argumenty a do historie šla zkomolená zpráva:

```powershell
git add <soubory>
git commit -F <cesta\k\zprave.txt>
git log -1 --format=%B   # over, ze zprava dosla cela
```
