# Solo compila el firmware (sin subir).
# Uso: .\scripts\compile.ps1

param(
    [string]$ArduinoCli = "C:\tools\Arduino-CLI\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$SketchDir = Join-Path $ProjectRoot "firmware"
$TimeLib = Join-Path $SketchDir "lib\Time-master"
$Fqbn = "arduino:avr:mega:cpu=atmega2560"

$Fso = New-Object -ComObject Scripting.FileSystemObject
$IncludeFlag = "-I$($Fso.GetFolder((Join-Path $SketchDir 'src')).ShortPath) -I$($Fso.GetFolder($SketchDir).ShortPath)"

& $ArduinoCli compile `
    --fqbn $Fqbn `
    --library $TimeLib `
    --build-property "compiler.cpp.extra_flags=$IncludeFlag" `
    --build-property "compiler.c.extra_flags=$IncludeFlag" `
    --output-dir "$ProjectRoot\output" `
    $SketchDir

if ($LASTEXITCODE -ne 0) {
    Write-Error "Falló la compilación."
}

Write-Host "Compilación OK."
