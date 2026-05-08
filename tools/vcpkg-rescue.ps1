<#
.SYNOPSIS
  Rescue a stalled vcpkg manifest install by auto-downloading any URL whose
  in-vcpkg curl failed with SSL error 35 (schannel CRL verify failure).

.DESCRIPTION
  vcpkg's bundled curl uses Windows schannel and cannot disable CRL checks.
  Many distros (sqlite.org, qt.io, etc.) host their certs behind CRLs that are
  themselves blocked in some networks, causing curl error 35 mid-download.

  This script:
    1. Parses the most recent vcpkg-manifest-install.log
    2. Extracts every "Downloading <URL>" that was followed by an error
    3. Re-downloads each URL using system curl.exe with --ssl-no-revoke
       through your proxy (default 127.0.0.1:7897)
    4. Drops the file at the exact name vcpkg expects
       ($env:LOCALAPPDATA\vcpkg\downloads\<filename>)
    5. Skips files that are already cached

  After running, just rerun cmake — vcpkg will pick up the cached files via
  SHA512 match and skip the failed downloads.

.PARAMETER LogPath
  Path to vcpkg-manifest-install.log. Defaults to the most recent one
  under build/*/vcpkg-manifest-install.log.

.PARAMETER Proxy
  Proxy URL (default http://127.0.0.1:7897 — Clash for Windows default).

.EXAMPLE
  pwsh -File tools/vcpkg-rescue.ps1

.EXAMPLE
  pwsh -File tools/vcpkg-rescue.ps1 -Proxy http://127.0.0.1:1080
#>

[CmdletBinding()]
param(
    [string] $LogPath = '',
    [string] $Proxy   = 'http://127.0.0.1:7897'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

# ---------- Locate the manifest log -----------------------------------------
if (-not $LogPath) {
    $candidates = Get-ChildItem -Path (Join-Path $repoRoot 'build') `
                                -Recurse -Filter 'vcpkg-manifest-install.log' `
                                -ErrorAction SilentlyContinue |
                  Sort-Object LastWriteTime -Descending
    if (-not $candidates) {
        Write-Error "No vcpkg-manifest-install.log found under $repoRoot\build\."
    }
    $LogPath = $candidates[0].FullName
}
Write-Host "[rescue] Using log: $LogPath" -ForegroundColor Cyan

# ---------- Parse the log ---------------------------------------------------
# Pattern: "Downloading <url> -> <localname>"  OR  "Downloading <url>"
# Followed within the next ~15 lines by either "Successfully downloaded ..."
# or "error: curl operation failed".
$lines  = Get-Content -Path $LogPath
$failed = New-Object System.Collections.Generic.List[object]

for ($i = 0; $i -lt $lines.Count; $i++) {
    $m = [regex]::Match($lines[$i], '^Downloading\s+(\S+?)(?:\s+->\s+(\S+))?\s*$')
    if (-not $m.Success) { continue }

    $url      = $m.Groups[1].Value
    $filename = if ($m.Groups[2].Success) { $m.Groups[2].Value }
                else { Split-Path $url -Leaf }

    # look ahead up to 30 lines to find the resolution
    $endIdx = [Math]::Min($i + 30, $lines.Count - 1)
    $window = $lines[$i..$endIdx] -join "`n"

    if ($window -match 'Successfully downloaded') { continue }
    if ($window -match 'error: curl operation failed' -or
        $window -match 'Download failed, halting portfile') {
        $failed.Add([pscustomobject]@{ Url = $url; FileName = $filename })
    }
}

if ($failed.Count -eq 0) {
    Write-Host "[rescue] No failed downloads in this log. Nothing to do." -ForegroundColor Green
    exit 0
}

Write-Host "[rescue] Found $($failed.Count) failed download(s):" -ForegroundColor Yellow
$failed | ForEach-Object { Write-Host "  - $($_.FileName)  ($($_.Url))" }

# ---------- Fetch each one --------------------------------------------------
# Prefer the project-configured downloads dir (preset's VCPKG_DOWNLOADS env)
# over the system-wide default. Falls back to %LOCALAPPDATA%\vcpkg\downloads.
$dlDir = if ($env:VCPKG_DOWNLOADS) { $env:VCPKG_DOWNLOADS }
         else { Join-Path $env:LOCALAPPDATA 'vcpkg\downloads' }
if (-not (Test-Path $dlDir)) {
    New-Item -ItemType Directory -Path $dlDir -Force | Out-Null
}
Write-Host "[rescue] Downloads dir: $dlDir" -ForegroundColor Cyan

$ok = 0; $skip = 0; $bad = 0
foreach ($f in $failed) {
    $dest = Join-Path $dlDir $f.FileName

    if ((Test-Path $dest) -and (Get-Item $dest).Length -gt 0) {
        Write-Host "[rescue] SKIP (already cached): $($f.FileName)" -ForegroundColor DarkGray
        $skip++
        continue
    }

    # remove any leftover .part files
    Get-ChildItem -Path $dlDir -Filter "$($f.FileName).*.part" -ErrorAction SilentlyContinue |
        Remove-Item -Force

    Write-Host "[rescue] FETCH $($f.FileName)" -ForegroundColor Cyan
    Write-Host "         from $($f.Url)"   -ForegroundColor DarkGray
    Write-Host "         via  $Proxy"      -ForegroundColor DarkGray

    & curl.exe `
        -x $Proxy `
        --ssl-no-revoke `
        -L `
        --connect-timeout 30 `
        --max-time 600 `
        --retry 3 `
        --retry-delay 5 `
        -o $dest `
        $f.Url

    if ($LASTEXITCODE -eq 0 -and (Test-Path $dest) -and (Get-Item $dest).Length -gt 0) {
        $size = (Get-Item $dest).Length
        Write-Host "         OK  ($([Math]::Round($size/1MB,2)) MB)" -ForegroundColor Green
        $ok++
    }
    else {
        Write-Host "         FAILED (curl exit $LASTEXITCODE)" -ForegroundColor Red
        if (Test-Path $dest) { Remove-Item $dest -Force }
        $bad++
    }
}

Write-Host ""
Write-Host "[rescue] Done.  rescued=$ok  skipped=$skip  failed=$bad" -ForegroundColor Cyan
if ($bad -gt 0) {
    Write-Host "[rescue] Some files still failed. Check proxy / try a different exit node." -ForegroundColor Yellow
    exit 1
}
Write-Host "[rescue] Now rerun:  cmake --preset my-vs2022-debug" -ForegroundColor Green
