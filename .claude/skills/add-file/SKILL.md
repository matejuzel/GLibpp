---
name: add-file
description: Add a new .cpp/.h source file to the GLibpp build correctly - vcxproj entry, vcxproj.filters entry, AdditionalIncludeDirectories for new directories (both x64 configs), UTF-8 BOM. Use whenever creating a new source file in this repo.
---

# Přidání nového souboru do buildu (GLibpp)

MSBuild negloboje — každý soubor se přidává ručně na **čtyři** místa. Vynechání kteréhokoli = soubor se nekompiluje, nenajde v IDE, nebo rozbije diakritiku.

## Checklist

1. **Vytvoř soubor s UTF-8 BOM** (konvence celého `GLibpp/src/`; `/utf-8` je zapnuté v obou konfiguracích). Nástroj Write BOM nezapisuje — po vytvoření ho doplň:
   ```powershell
   $p = "GLibpp\src\...\NovySoubor.h"; $b = [System.IO.File]::ReadAllBytes($p); if ($b.Length -lt 3 -or $b[0] -ne 0xEF) { [System.IO.File]::WriteAllText($p, [System.Text.Encoding]::UTF8.GetString($b), (New-Object System.Text.UTF8Encoding $true)) }
   ```
   Komentáře v souboru česky (bez diakritiky je v pořádku, většina kódu ji nepoužívá).

2. **`GLibpp/GLibpp.vcxproj`** — přidej do správného ItemGroup (abecedně mezi sousedy):
   - hlavička: `<ClInclude Include="src\...\NovySoubor.h" />`
   - zdroják: `<ClCompile Include="src\...\NovySoubor.cpp" />`

3. **`GLibpp/GLibpp.vcxproj.filters`** — přidej odpovídající entry s filtrem. Názvy filtrů jsou české (`Zdrojové soubory\Engine\Renderer` apod.) — okopíruj filtr od souseda ze stejného adresáře. Filters mají vliv jen na IDE, ale drž je konzistentní.

4. **Nový adresář?** Přidej ho do `AdditionalIncludeDirectories` v **obou** `ItemDefinitionGroup` (`Debug|x64` **i** `Release|x64`) v `GLibpp.vcxproj`. Includes jsou **ploché** — kód píše `#include "NovySoubor.h"`, nikdy relativní cestu; bez include diru se soubor nenajde.

5. **Ověř buildem** (`Debug|x64`) — viz skill `verify`.

## Poznámky

- `Win32`/`x86` konfigurace se neupravují — jsou nefunkční záměrně.
- Nikdy nepřidávej nic z `GLibpp/_old/`.
