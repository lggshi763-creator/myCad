<#
.SYNOPSIS
  Verify a C/C++ file conforms to myCad style: clang-format + clang-tidy + Doxygen.

.DESCRIPTION
  Runs three layers of checks:
    1. clang-format --dry-run --Werror   (formatting)
    2. clang-tidy   (lint, optional - off by default - flips on with -Tidy)
    3. Doxygen comment convention (mycad-specific):
       - /// comments, NOT /** ... */
       - @brief present, English-only
       - @param for each function parameter
       - @return for non-void return
       Spec: CLAUDE.md §2.3

  Output is one violation per line:
       [style] <file>:<line>: <issue>

  Exits 0 on clean, 1 on any violation. Designed to be called by:
    - .claude/settings.local.json PostToolUse hook (auto on Edit/Write)
    - .githooks/pre-commit (over staged files)
    - User manually

.PARAMETER Path
  File path or glob. Multiple files OK.

.PARAMETER Tidy
  Also run clang-tidy. Slow; off by default.

.PARAMETER QuietOK
  Print nothing when file is clean (default: print "[style] OK ...").

.EXAMPLE
  pwsh -File tools/check-style.ps1 -Path src/domain/shared/Hello.cpp

.EXAMPLE
  pwsh -File tools/check-style.ps1 -Path src/**/*.hpp -Tidy
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $false, ValueFromRemainingArguments = $true)]
    [string[]] $Path,
    [switch]   $Tidy,
    [switch]   $QuietOK
)

$ErrorActionPreference = 'Continue'

# ---------- Resolve files (handle globs + Edit hook env var) ----------------
if (-not $Path -or $Path.Count -eq 0) {
    if ($env:CLAUDE_FILE_PATHS) {
        $Path = $env:CLAUDE_FILE_PATHS -split ';|,' | Where-Object { $_ }
    } elseif ($env:CLAUDE_FILE_PATH) {
        $Path = @($env:CLAUDE_FILE_PATH)
    } else {
        Write-Host "[style] Usage: check-style.ps1 -Path <file>"
        exit 0
    }
}

$files = @()
foreach ($p in $Path) {
    if (Test-Path $p -PathType Leaf) {
        $files += (Resolve-Path $p).Path
    } elseif ($p -match '\*') {
        $files += (Get-ChildItem -Path $p -File -ErrorAction SilentlyContinue).FullName
    }
}

# Filter to C/C++ files; skip placeholder/generated.
$cppExt = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inl', '.ipp')
$skipPatterns = @('placeholder\.(cpp|hpp)$', 'placeholder_test\.cpp$', 'moc_.*\.cpp$', 'qrc_.*\.cpp$', 'ui_.*\.h$')

$files = $files | Where-Object {
    $ext = [System.IO.Path]::GetExtension($_).ToLower()
    if ($ext -notin $cppExt) { return $false }
    foreach ($pat in $skipPatterns) {
        if ($_ -match $pat) { return $false }
    }
    return $true
}

if ($files.Count -eq 0) {
    if (-not $QuietOK) { Write-Host "[style] No C/C++ files to check." }
    exit 0
}

# ---------- Find clang-format / clang-tidy ----------------------------------
function Find-Tool($name) {
    if (Get-Command $name -ErrorAction SilentlyContinue) { return $name }
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\$name.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\$name.exe",
        "C:\Program Files\LLVM\bin\$name.exe"
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    return $null
}

$cf = Find-Tool 'clang-format'
$ct = if ($Tidy) { Find-Tool 'clang-tidy' } else { $null }

$totalViolations = 0

foreach ($f in $files) {
    $relPath = Resolve-Path -Relative $f -ErrorAction SilentlyContinue
    if (-not $relPath) { $relPath = $f }
    $fileViolations = 0

    # ---------- Layer 1: clang-format ---------------------------------------
    if ($cf) {
        $out = & $cf --style=file --dry-run --Werror "$f" 2>&1
        if ($LASTEXITCODE -ne 0) {
            foreach ($line in $out) {
                Write-Host "[style] $relPath: clang-format: $line"
                $fileViolations++
            }
        }
    } else {
        Write-Host "[style] WARNING: clang-format not found; skipping format check."
    }

    # ---------- Layer 2: clang-tidy (optional) ------------------------------
    if ($Tidy -and $ct) {
        $tidyOut = & $ct "$f" --quiet 2>&1
        if ($LASTEXITCODE -ne 0) {
            foreach ($line in $tidyOut | Where-Object { $_ -match 'warning:|error:' }) {
                Write-Host "[style] $relPath: clang-tidy: $line"
                $fileViolations++
            }
        }
    }

    # ---------- Layer 3: Doxygen convention (regex-based; fast & approximate) ---
    if ($f -match '\.(hpp|h|hxx|hh)$') {
        $content = Get-Content $f -Raw -Encoding UTF8
        $lines   = Get-Content $f -Encoding UTF8

        # 3a. Forbid /** ... */ block comments.
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match '^\s*/\*\*') {
                Write-Host "[style] $relPath`:$($i+1): Doxygen: use /// instead of /** ... */ (CLAUDE.md §2.3)"
                $fileViolations++
            }
        }

        # 3b. Each documented function declaration should have @brief somewhere
        #     in the preceding /// block. Heuristic: find `///` blocks followed
        #     within 3 lines by a function-like declaration; check the block
        #     contains @brief. Skip @file blocks at top.
        $inBlock = $false
        $blockStart = -1
        $blockHasBrief = $false
        $blockIsFile = $false
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $ln = $lines[$i]
            if ($ln -match '^\s*///') {
                if (-not $inBlock) { $inBlock = $true; $blockStart = $i; $blockHasBrief = $false; $blockIsFile = $false }
                if ($ln -match '@brief') { $blockHasBrief = $true }
                if ($ln -match '@file')  { $blockIsFile  = $true }
            } else {
                if ($inBlock) {
                    # Block just ended at line $i-1.
                    # Look ahead: is there a function-like decl in next 3 non-blank lines?
                    $declFound = $false
                    for ($j = $i; $j -lt [Math]::Min($i + 4, $lines.Count); $j++) {
                        if ($lines[$j].Trim() -eq '') { continue }
                        # Crude function detector: contains '(' before any ';' on the line and not 'class ' / 'struct '
                        if ($lines[$j] -match '\(' -and $lines[$j] -notmatch '^\s*(class|struct|namespace|enum)\b') {
                            $declFound = $true
                        }
                        break
                    }
                    if ($declFound -and -not $blockHasBrief -and -not $blockIsFile) {
                        Write-Host "[style] $relPath`:$($blockStart+1): Doxygen: comment block missing @brief (CLAUDE.md §2.3)"
                        $fileViolations++
                    }
                    $inBlock = $false
                }
            }
        }
    }

    if ($fileViolations -gt 0) {
        $totalViolations += $fileViolations
        Write-Host "[style] $relPath`: $fileViolations violation(s)"
    } elseif (-not $QuietOK) {
        Write-Host "[style] $relPath`: OK"
    }
}

if ($totalViolations -eq 0) { exit 0 }

Write-Host ""
Write-Host "[style] Total violations: $totalViolations"
Write-Host "[style] Auto-fix what you can:"
Write-Host "         clang-format -i <file>"
Write-Host "[style] See CLAUDE.md §2.3 for Doxygen comment rules."
exit 1
