# tools/vcpkg-reset-registry.ps1
# 清除 vcpkg registry 的 unborn-master 坏状态（W19 复发 3 次的根因）
# 用法：. .\tools\vcpkg-reset-registry.ps1
$cache = "$env:LOCALAPPDATA\vcpkg\registries"
Get-Process TGitCache -ErrorAction SilentlyContinue | Stop-Process -Force
Remove-Item -Recurse -Force "$cache\git*" -ErrorAction SilentlyContinue
Write-Host "vcpkg registry cache cleared. Next cmake will re-clone (~1-3 min, need proxy)."
