---
name: glib-check-encoding
description: Guard against encoding corruption (mojibake) in GLibpp sources - verify UTF-8 BOM presence, valid UTF-8 bytes and absence of U+FFFD replacement characters in changed files. Run before every commit and after editing files with Czech comments.
---

# Kontrola kódování zdrojáků (GLibpp)

Nejnebezpečnější past v repu: historicky byly soubory Windows-1250 bez BOM; přečtení jako UTF-8 změní diakritiku na `U+FFFD` (`�`) a zpětný zápis **nenávratně zničí text** (`Změří čas` → `Zm��� �as`). Vše pod `GLibpp/src/` musí být UTF-8 **s BOM**.

## Použití

```powershell
& ".claude\skills\glib-check-encoding\check-encoding.ps1"
```

Bez parametrů zkontroluje všechny změněné/nové `.h`/`.cpp` soubory podle gitu (proti HEAD). Konkrétní soubory: `-Paths "GLibpp\src\A.h","GLibpp\src\B.cpp"`. Exit 0 = čisté, exit 1 = nález (vypíše co a kde).

## Co kontroluje

1. **BOM** — první 3 bajty musí být `EF BB BF`.
2. **Validní UTF-8** — striktní dekódování; nevalidní bajty = soubor je pravděpodobně pořád CP1250 → needitovat, nejdřív překódovat.
3. **`U+FFFD` (`�`)** — pokud je přítomen, text už byl zničen → **needituj**, obnov řádek z gitu: `git show HEAD:<cesta>` dekódované jako CP1250.

## Výjimky

- `GLibpp/_old/` — reference-only, záměrně CP1250, nikdy neopravovat (skript ho přeskakuje).
- Větev `worktree-remove_jitter` má mojibake už zacommitnuté v `Renderer.h`.
