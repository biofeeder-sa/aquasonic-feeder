# Compila y sube el firmware Aquasonic al Arduino Mega 2560.
# Uso:
#   .\scripts\compile-upload.ps1
#   .\scripts\compile-upload.ps1 -Port COM4

param(
    [string]$Port = "COM3",
    [string]$ArduinoCli = "C:\tools\Arduino-CLI\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$SketchDir = Join-Path $ProjectRoot "firmware"
$TimeLib = Join-Path $SketchDir "lib\Time-master"
$Fqbn = "arduino:avr:mega:cpu=atmega2560"

if (-not (Test-Path $ArduinoCli)) {
    Write-Error "No se encontró arduino-cli en: $ArduinoCli"
}

if (-not (Test-Path $SketchDir)) {
    Write-Error "No se encontró la carpeta del sketch: $SketchDir"
}

Write-Host "==> Placa:  $Fqbn"
Write-Host "==> Puerto: $Port"
Write-Host "==> Sketch: $SketchDir"
Write-Host ""

$Fso = New-Object -ComObject Scripting.FileSystemObject
$IncludeFlag = "-I$($Fso.GetFolder((Join-Path $SketchDir 'src')).ShortPath) -I$($Fso.GetFolder($SketchDir).ShortPath)"

& $ArduinoCli compile `
    --fqbn $Fqbn `
    --library $TimeLib `
    --build-property "compiler.cpp.extra_flags=$IncludeFlag" `
    --build-property "compiler.c.extra_flags=$IncludeFlag" `
    --upload `
    -p $Port `
    $SketchDir

if ($LASTEXITCODE -ne 0) {
    Write-Error "Falló la compilación o la carga del firmware."
}

Write-Host ""
Write-Host "Firmware cargado correctamente en $Port."
