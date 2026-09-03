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

## Variables de configuración y alarmas (bits)

Las variables wire **`0x0C 0x03`** (*Setting Bytes*), **`0x0D 0xBE`** (*Alarms*) y **`0x0D 0xB5`** (*Alarm Mask*) son 4 bytes cada una. En el firmware se accede como `VAR_WIRE_BYTE(var, 2)` … `VAR_WIRE_BYTE(var, 5)` (bytes de valor, no el ID).

Convención de bits: **bit 0 = LSB** del byte. Valor `1` = condición activa / habilitada.

### Setting Bytes (`VAR_SETTING_BYTES` — `0x0C 0x03`)

| Byte | Bit   | Default | Descripción                                                                 |
|------|-----  |---------|-----------------------------------------------------------------------------|
| 0    | 0–6   | 0       | *Reservado* (no usado en firmware actual).                                  |
| 0    | **7** | 0       | **LED de estado:** si `1`, parpadea el LED (`D13`) cada segundo con RTC     |
|      |       |         | sincronizado Si `0`, LED apagado. **Escritura RTC:** si `1`, bloquea la     |
|      |       |         | actualización remota del RTC (`0x0D 0xD8`); si `0`, permite escribir hora   |
|      |       |         | desde el servidor.                                                          |
|------|-------|---------|-----------------------------------------------------------------------------|
| 1    | 0–3   | 0       | *Reservado* (no usado en firmware actual).                                  |
| 1    | **4** | 1       | **Modo dosificación por vueltas:** si `1`, la dosificación termina al       |
|      |       |         | alcanzar `N` vueltas (`VAR_N_VUELTAS`) contadas por corriente en X2;        |
|      |       |         | habilita conteo de picos, detección de inactividad y calibración remota.    |
|      |       |         | Si `0`, la dosificación termina por **tiempo** (`feed_rate × gramos_ciclo`).|
| 1    | 5–7   | 0       | *Reservado* (bit 3 aparece en código comentado, sin efecto activo).         |
|------|-------|---------|-----------------------------------------------------------------------------|
| 2    | 0–6   | 0       | *Reservado*.                                                                |
| 2    | **7** | 1       | **Alimentador habilitado:** si `1`, permite dosificación automática y       |
|      |       |         | alterna salida `X0_1` en modo activo. Si `0`, modo standby: no dosifica,    |
|      |       |         | apaga motores si X3 estaba encendido, alterna `X0_2` y LED standby durante  |
|      |       |         | arranque sin RTC.                                                           |
|------|-------|---------|-----------------------------------------------------------------------------|
| 3    | 0–7   | 0       | *Reservado* (no usado en firmware actual).                                  |
|------|-------|---------|-----------------------------------------------------------------------------|

**Default de fábrica:** `00 10 80 00`.

---

### Alarmas (`VAR_ALARMS` — `0x0D 0xBE`)

Estado en tiempo real de eventos y fallos. Solo lectura remota (el firmware las actualiza).

Los cuatro bytes de valor se acceden como `VAR_WIRE_BYTE(VAR_ALARMS, 2)` … `5`. En las tablas, **índice** es ese subíndice; **byte lógico** sigue la convención histórica del protocolo (3 → 0).

#### Byte lógico 3 (índice 2) — Desgaste y cambio de motor (ACS)

| Bit   | Motor             | Descripción                                                       |
|-------|-------------------|-------------------------------------------------------------------|
| **0** | X2 (aspersor)     | **Desgaste:** `baseline_slow` ≥ `ref × (1 + AC 0x19%)`.           | 
|       |                   | Se limpia tras 2 ciclos sanos con `I_max` bajo umbral.            |
| **1** | X3 (dosificador)  | **Desgaste:** `baseline_slow` ≥ `ref × (1 + AC 0x0A%)`.           |
|       |                   | Misma lógica de clear.                                            |
| **2** | X2                | **Cambio de motor detectado:** auto-reset pendiente de confirmación |
|       |                   | (`0xC0`). Se limpia al aceptar/revertir o con `AC 0x1D = 0x01`.   |
| **3** | X3                | **Cambio de motor detectado.** Se limpia con `0xC0` o `AC 0x18`.  |
| **4** | X2                | **Aspa desprendida:** sin vueltas + `I_max` ≪ referencia          |
|       |                   | X2 (≈ &lt; 25 % de `ref`). Se distingue de tolva vacía            |   
|       |                   | (~60 % de `ref`). Latch tras 2 ciclos; clear con 2 ciclos sanos   |
|       |                   | o `AC 0x20 = 0x01`.                                               |
| 5–7   |         —         | *Sin uso*.                                                        |

Las alarmas de **desgaste** reflejan el estado latcheado del monitor (`slow` vs referencia). Las de **cambio de motor** se activan en auto-detección pendiente de confirmación (no en reset remoto voluntario). Ver trama **`0xC0`** más abajo.

Los ciclos con **tolva vacía al final** (bit 6, byte índice 5) se descartan para desgaste, cambio de motor, comisionamiento y runtime; además reinician las rachas de detección de perfil/caída.

#### Byte lógico 2 (índice 3) — Ciclo de alimentación y energía

| Bit   | Descripción                                                                                |
|-------|--------------------------------------------------------------------------------------------|
| 0–2   | *Sin uso*.                                                                                 |
| **3** | **Ciclo de dosificación completado:** alimentación ejecutada (modo eventos o tabla). Se    |
|       | limpia al recibir ACK del servidor. Impide iniciar un nuevo ciclo en modo eventos mientras |
|       | esté activa.                                                                               |
| **4** | **Batería baja:** voltaje de batería ≤ umbral `0x0D 0x87` (`VAR_LOW_BATTERY`) con          |
|       | histéresis de recuperación.                                                                |
| 5–6   | *Sin uso*.                                                                                 |
| **7** | **Vueltas incompletas:** en modo por vueltas, el ciclo terminó con `0 < nPicos < objetivo` |
|       | (detención anticipada o inactividad).                                                      |

#### Byte lógico 1 (índice 4) — Comunicación

| Bit   | Descripción                                                                                |
|-------|--------------------------------------------------------------------------------------------|
| 0–2   | *Sin uso* (bits 0–1 referenciados en código comentado de balanza).                         |
| **3** | **Timeout de comunicación:** sin actividad XBee durante el tiempo configurado en           |
|       | `0x00 0x17` (`VAR_TIME_ALARM`, minutos). Se limpia al recibir ACK.                         |
| 4–7   | *Sin uso*.                                                                                 |

#### Byte lógico 0 (índice 5) — Protección y sensores de motor (ACS)

| Bit   | Motor | Descripción                                                                        |
|-------|-------|------------------------------------------------------------------------------------|
| **0** | X1    | **Desconectado / sin carga:** corriente ≤ `AC 0x07` (`VAR_DISCONNECTED`).          |
| **1** | X2    | **Desconectado / sin carga:** corriente ≤ `VAR_DISCONNECTED`.                      |
| **2** | X3    | **Desconectado / sin carga:** corriente ≤ `VAR_DISCONNECTED`.                      |
| **3** | X1    | **Protección por sobrecorriente:** corriente ≥ `AC 0x04` (`VAR_PROT_X1` × 100 cA). |
| **4** | X2    | **Protección por sobrecorriente:** corriente ≥ `AC 0x05`; apaga X2.                |
| **5** | X3    | **Protección por sobrecorriente:** corriente ≥ `AC 0x06`; apaga X3.                |
| **6** | X2    | **Tolva vacía / sin balanceado:** corriente de X2 en rango de hopper vacío         |
|       |       | (`AC 0x08`) al **final** de la dosificación.                                       |
| **7** | —     | *Sin uso*.                                                                         |

---

### Máscara de alarmas (`VAR_ALARM_MASK` — `0x0D 0xB5`)

Misma disposición de bits que `VAR_ALARMS`. Cada bit habilita (`1`) o suprime (`0`) el reporte y, en varios casos, la generación de la alarma correspondiente.

| Byte (índice) | Bits activos (default) | Efecto |
|---------------|------------------------|--------|
| 0 (índice 2) | `0x1F` (bits 0–4) | Habilita desgaste X2/X3, cambio motor X2/X3 y aspa desprendida X2. |
| 1 (índice 3) | `0x28` (bits 3, 5) | Habilita alarma de ciclo completado (bit 3). Bit 5 sin función asociada en firmware actual. |
| 2 (índice 4) | `0x00` | Sin alarmas habilitadas en este byte. |
| 3 (índice 5) | `0x76` (bits 1,2,4,5,6) | Habilita desconectado X2/X3, protección X2/X3 y tolva vacía. |

Si un bit de máscara está en `0`, el firmware fuerza a `0` el bit equivalente en `VAR_ALARMS` (`bitDisabledACS` / `motorWearApplyAlarmMask`).

**Default de fábrica:** `1F 28 00 76`.

---

### Variables ACS — monitor de desgaste (referencia rápida)

Unidades de corriente: **centiamperios (cA)** — p. ej. 140 mA = 14 cA.

#### X3 — dosificador

| ID wire   | Variable                | Uso |
|-----------|-------------------------|-----|
| `AC 0x09` | `VAR_MOTOR_CHANGE_PCT` | Umbral de salto para detección de cambio de motor por perfil (% sobre `ref`, **compartido X2/X3**, default **40**, rango 1–255). |
| `AC 0x0C` | `VAR_MOTOR_REF_SAMPLES`| Ciclos válidos para calcular `ref` al comisionar (**compartido X2/X3**, default **20**, rango 1–255). |
| `AC 0x0A` | `VAR_X3_WEAR_PCT`       | Umbral de desgaste (% sobre referencia, default 30, rango 1–255). |
| `AC 0x0B` | `VAR_X3_WEAR_EMA_ALPHA` | Factor α de la EMA de `slow` (% por ciclo, default 2, rango 1–100). |
| `AC 0x16` | `VAR_X3_BASELINE_REF`   | Referencia de corriente nominal (solo lectura remota). |
| `AC 0x17` | `VAR_X3_BASELINE_SLOW`  | Baseline lenta EMA (solo lectura). |
| `AC 0x18` | `VAR_X3_MOTOR_RESET`    | Escritura `0x01` = reset / recommissioning; limpia alarma de cambio de motor y runtime X3. |

#### X2 — aspersor

| ID wire   | Variable                | Uso |
|-----------|-------------------------|-----|
| `AC 0x19` | `VAR_X2_WEAR_PCT`       | Umbral de desgaste (default 30). |
| `AC 0x1A` | `VAR_X2_WEAR_EMA_ALPHA` | Factor α EMA (default 2). |
| `AC 0x1B` | `VAR_X2_BASELINE_REF`   | Referencia (solo lectura). |
| `AC 0x1C` | `VAR_X2_BASELINE_SLOW`  | Baseline lenta (solo lectura). |
| `AC 0x1D` | `VAR_X2_MOTOR_RESET`    | Escritura `0x01` = reset / recommissioning; limpia alarma de cambio de motor y runtime X2. |
| `AC 0x1E` | `VAR_X2_BLADE_AMP_PCT`  | Umbral aspa off: `I_max` &lt; `ref × pct%` (default **25**, rango 10–45). |
| `AC 0x1F` | `VAR_X2_BLADE_MIN_SWING`| Máximo swing `(max−min)` en cA para confirmar curva plana (default **15**). |
| `AC 0x20` | `VAR_X2_BLADE_ACK`      | Escritura `0x01` = limpiar alarma de aspa desprendida. |
| `AC 0x21` | `VAR_X2_MOTOR_RUNTIME`  | Segundos de funcionamiento X2 acumulados desde último reset/cambio de motor (solo lectura remota, EEPROM). No suma ciclos con tolva vacía al final. |
| `AC 0x22` | `VAR_X3_MOTOR_RUNTIME`  | Segundos de funcionamiento X3 acumulados (misma lógica). Horas ≈ valor / 3600. |

#### Trama de confirmación de cambio de motor (`0xC0`)

Payload Biofeeder dentro de API XBee `0x90` (índice base `txFrame[15]`):

| Campo | Bytes | Descripción |
|-------|-------|-------------|
| cmd | 1 | `0xC0` |
| frame id | 2 | MSB, LSB |
| motor | 1 | `0` = aspersor (X2); `1` = dosificador (X3) |
| acción | 1 | `0` = aceptar cambio; `1` = revertir |
| reservado | 2 | `0x00 0x00` |

Respuesta: `0x05` + frame id + resultado (`0x00` = OK, `0x01` = error).

En auto-detección, `ref`, `slow` y runtime previos se respaldan en EEPROM interna. **Aceptar** descarta el respaldo y mantiene el estudio nuevo; **revertir** restaura los valores anteriores.

#### Otras ACS usadas por el monitor

| ID wire   | Variable           | Uso |
|-----------|--------------------|-----|
| `AC 0x07` | `VAR_DISCONNECTED` | Umbral mínimo de corriente válida (cA). |
| `AC 0x08` | `VAR_EMPTY_HOPPER` | Umbral tolva vacía (cA); auto-calibración al inicio de ciclo. |
| `AC 0x09` | `VAR_MOTOR_CHANGE_PCT` | Umbral de salto cambio de motor por perfil (% sobre `ref`, X2 y X3, default 40). |
| `AC 0x0C` | `VAR_MOTOR_REF_SAMPLES`| Muestras para calcular `ref` al comisionar (X2 y X3, default 20). |
| `AC 0x05` | `VAR_PROT_X2`      | Protección X2 (× 100 cA). |
| `AC 0x06` | `VAR_PROT_X3`      | Protección X3 (× 100 cA). |

## Hardware

- Esquemático: `hardware/schematics/bioMatic_v1.pdf`
- Imágenes de placa: `hardware/board/`
