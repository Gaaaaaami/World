$files = @(
  "D:\Users\Administrator\Documents\Unreal Projects\World\Plugins\VoxelTerrain\Source\VoxelTerrain\Private\VoxelGenerator.cpp",
  "D:\Users\Administrator\Documents\Unreal Projects\World\Plugins\VoxelTerrain\Source\VoxelTerrain\Public\VoxelGenerator.h"
)

foreach ($f in $files) {
    if (-not (Test-Path $f)) { Write-Host "跳过（不存在）: $f"; continue }
    $bytes = [System.IO.File]::ReadAllBytes($f)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $newBytes = New-Object byte[] ($bytes.Length - 3)
        [Array]::Copy($bytes, 3, $newBytes, 0, $newBytes.Length)
        [System.IO.File]::WriteAllBytes($f, $newBytes)
        Write-Host "✅ 已删除 BOM: $f"
    } else {
        Write-Host "ℹ️ 无 BOM: $f（前 3 字节: $($bytes[0..2] | ForEach-Object { '{0:X2}' -f $_ } -join ' ')）"
    }
}