# Valida y regenera el registro de variables. Falla si hay IDs o direcciones EEPROM duplicadas.
$ErrorActionPreference = "Stop"

$Script = Join-Path (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)) "firmware\tools\gen_var_registry.py"
python $Script
if ($LASTEXITCODE -ne 0) {
    Write-Error "Validacion del registro de variables fallo."
}
Write-Host "Registro de variables OK."
