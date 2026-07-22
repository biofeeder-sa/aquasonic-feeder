# aquasonic-feeder-ard

Firmware del alimentador **Aquasonic** para ATmega 2560.

Proyecto independiente (sin vínculo al repositorio histórico de GitLab).

**Base de código:** `v00.00.27` (rama `dev00.00.27` de biomatic-feeder). A partir de aquí se trabaja solo en este directorio.

## Estructura

```
aquasonic-feeder-ard/
├── firmware/
│   ├── firmware.ino          # setup() y loop()
│   ├── src/                  # Módulos del firmware
│   │   ├── core/             # config, app_state, vars.h
│   │   ├── variables/        # Registro wire + accessors
│   │   ├── comm/             # Parser y buffers XBee
│   │   ├── state/            # rtc_cache
│   │   ├── actuators/, rtc/, dosage/, eeprom/, sensors/, xbee/, acs/
│   ├── tools/                # gen_var_registry.py; archive/ = migración histórica
│   └── lib/                  # Time-master (RTC)
├── hardware/                 # Esquemáticos e imágenes de placa
├── docs/
├── scripts/                  # compile.ps1, compile-upload.ps1
└── README.md
```

## Características

- 2 entradas analógicas (batería y panel solar)
- 4 entradas analógicas de corriente
- 4 entradas analógicas 4-20 mA
- 1 sensor de peso HX711
- 5 salidas a 12 V
- Socket XBee

## Requisitos

- [Arduino IDE](https://www.arduino.cc/en/software) (o extensión Arduino en VS Code/Cursor)
- Placa: **Arduino Mega 2560** (`arduino:avr:mega`, `cpu=atmega2560`)

## Uso

1. Abre la carpeta `aquasonic-feeder-ard` en el IDE.
2. Compila / sube el sketch `firmware/firmware.ino`.
3. Los hex generados quedan en `output/` (si usas la extensión Arduino de VS Code).

### Scripts (Arduino CLI)

Puerto por defecto: **COM3** (Mega 2560).

```powershell
# Compilar y subir a COM3
.\scripts\compile-upload.ps1

# Compilar y subir a otro puerto
.\scripts\compile-upload.ps1 -Port COM4

# Solo compilar
.\scripts\compile.ps1

# Validar/regenerar registro de variables
.\scripts\validate_registry.ps1
```

Requisito: `C:\tools\Arduino-CLI\arduino-cli.exe` con el core `arduino:avr` instalado.

## Hardware

- Esquemático: `hardware/schematics/bioMatic_v1.pdf`
- Imágenes de placa: `hardware/board/`
