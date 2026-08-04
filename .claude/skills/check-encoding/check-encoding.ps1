# Kontrola kodovani zdrojaku GLibpp: BOM, validni UTF-8, absence U+FFFD.
# Bez parametru kontroluje zmenene/nove .h/.cpp podle gitu (proti HEAD).
param([string[]]$Paths)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
Set-Location $repoRoot

if (-not $Paths) {
    $changed = @(git diff --name-only HEAD) + @(git ls-files --others --exclude-standard)
    $Paths = $changed | Where-Object { $_ -match '\.(h|cpp)$' -and $_ -notmatch '_old/' } | Sort-Object -Unique
}

if (-not $Paths -or $Paths.Count -eq 0) { Write-Output "OK: zadne soubory ke kontrole"; exit 0 }

$fail = $false
foreach ($rel in $Paths) {
    if ($rel -match '_old[\\/]') { continue }
    $p = Join-Path $repoRoot $rel
    if (-not (Test-Path $p)) { continue }   # smazany soubor

    $bytes = [System.IO.File]::ReadAllBytes($p)

    # 1) BOM
    if ($bytes.Length -lt 3 -or $bytes[0] -ne 0xEF -or $bytes[1] -ne 0xBB -or $bytes[2] -ne 0xBF) {
        Write-Output "CHYBI BOM: $rel"
        $fail = $true
    }

    # 2) validni UTF-8 (striktni dekodovani vyhodi vyjimku na nevalidnich bajtech)
    $strict = New-Object System.Text.UTF8Encoding($true, $true)
    $text = $null
    try {
        $text = $strict.GetString($bytes)
    } catch {
        Write-Output "NEVALIDNI UTF-8 (nejspis CP1250): $rel - NEEDITOVAT, nejdriv prekodovat"
        $fail = $true
        continue
    }

    # 3) U+FFFD = uz znicena diakritika
    if ($text.IndexOf([char]0xFFFD) -ge 0) {
        $lineNo = 0
        foreach ($line in $text -split "`n") {
            $lineNo++
            if ($line.IndexOf([char]0xFFFD) -ge 0) {
                Write-Output ("U+FFFD (znicena diakritika): {0}:{1} - NEEDITOVAT, obnovit z gitu jako CP1250" -f $rel, $lineNo)
            }
        }
        $fail = $true
    }
}

if ($fail) { exit 1 }
Write-Output ("OK: {0} souboru cistych" -f $Paths.Count)
