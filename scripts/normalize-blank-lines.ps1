# Normaliza lineas en blanco redundantes en firmware/src (espaciado "una linea si, una no").
param(
    [string]$Root = (Join-Path (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)) "firmware")
)

function Normalize-SourceLines {
    param([string[]]$Lines)

    $result = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        $line = $Lines[$i]
        $isEmpty = [string]::IsNullOrWhiteSpace($line)
        if ($isEmpty) {
            $prevNonEmpty = ($i -gt 0) -and (-not [string]::IsNullOrWhiteSpace($Lines[$i - 1]))
            $nextNonEmpty = ($i -lt ($Lines.Count - 1)) -and (-not [string]::IsNullOrWhiteSpace($Lines[$i + 1]))
            if ($prevNonEmpty -and $nextNonEmpty) { continue }
            if ($result.Count -gt 0 -and [string]::IsNullOrWhiteSpace($result[$result.Count - 1])) { continue }
        }
        [void]$result.Add($line)
    }
    while ($result.Count -gt 0 -and [string]::IsNullOrWhiteSpace($result[$result.Count - 1])) {
        $result.RemoveAt($result.Count - 1)
    }
    return ,$result.ToArray()
}

$files = @()
$files += Get-ChildItem -Path (Join-Path $Root 'src') -Recurse -Include *.cpp,*.h -File
$ino = Join-Path $Root 'firmware.ino'
if (Test-Path $ino) { $files += Get-Item $ino }

$changed = 0
foreach ($file in $files) {
    $before = Get-Content -LiteralPath $file.FullName
    $after = Normalize-SourceLines -Lines $before
    if ($before.Count -ne $after.Count) {
        [System.IO.File]::WriteAllLines($file.FullName, $after)
        $changed++
        Write-Host "normalized: $($file.Name)"
    }
}

Write-Host "Total files changed: $changed"
