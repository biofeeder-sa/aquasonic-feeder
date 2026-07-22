# Visión general

Firmware del alimentador Aquasonic basado en ATmega 2560.

**Versión de partida:** software `00.00.27` (sincronizado desde `biomatic-feeder` / `dev00.00.27`).

## Estructura modular

El sketch principal (`firmware.ino`) contiene solo `setup()` y `loop()`. La lógica vive en `firmware/src/`:

| Módulo | Responsabilidad |
|--------|-----------------|
| `core/config.h` | Pines, estados lógicos, constantes del protocolo XBee |
| `core/app_state.*` | Estado runtime: `app` (flags + `dosageRt`) |
| `comm/xbee_buffers.*` | Buffers XBee, parser stats, `xbee.comm` |
| `acs/acs_sensor.*` | Sensores ACS712: struct `acs` |
| `core/vars.h` | Include maestro del firmware |
| `variables/` | Registro wire (`transferVar`), accessors y callbacks EEPROM |
| `comm/` | Parser API XBee no bloqueante |
| `state/rtc_cache.*` | Caché de tiempo y sombras del RTC |
| `actuators.*` | PWM, arranque de motores, LEDs |
| `rtc.*` | Reloj interno, BCD, tablas horarias |
| `dosage.*` | Dosificación por feed-rate y calibración |
| `eeprom_*` | Persistencia EEPROM y bootstrap de config |
| `sensors.*` | Batería, panel, peso HX711 |
| `xbee.*` | Frames API, broadcast, unicast, alarmas |

Herramientas en `firmware/tools/`:
- `gen_var_registry.py` — genera `GlobalVariables.h/cpp`
- `archive/` — scripts de migración histórica (no usados en build)

## Flujo principal (`loop`)

1. `Send_Sequence_Broadcast()` — presencia en red XBee
2. `xbee_communication()` — parser serial XBee
3. `Sense_Voltajes()` — batería y panel solar
4. `calibration()` — modo calibración remoto
5. Si RTC sincronizado: reloj, dosificación, alarmas

## Notas de compilación

- Placa: **Arduino Mega 2560**
- Arduino CLI compila todos los `.cpp` bajo `firmware/`
- Librería de tiempo activa: `lib/Time-master` (ver `scripts/compile.ps1`)
