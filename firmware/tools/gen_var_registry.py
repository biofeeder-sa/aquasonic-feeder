#!/usr/bin/env python3
"""Generate src/variables/GlobalVariables.h and .cpp from feeder registry metadata."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "src" / "variables"
OUT_H = OUT_DIR / "GlobalVariables.h"
OUT_CPP = OUT_DIR / "GlobalVariables.cpp"

META_COLS = 9
VAR_VALUE = 9

# (name, id_msb, id_lsb, size, format, eeprom, eep_msb, eep_lsb, wr, log_min, values...)
ROWS = [
    ("VAR_SETTING_BYTES",      0x0C, 0x03, 4, 0x05, 1, 0x00, 0x00, 1, 0, [0x00, 0x10, 0x80, 0x00]),
    ("VAR_SOFTWARE_VERSION",   0x0C, 0x07, 8, 0x08, 0, 0x00, 0x00, 0, 0, [0x30, 0x30, 0x2E, 0x30, 0x30, 0x2E, 0x32, 0x37]),
    ("VAR_FEED_RATE",          0x0C, 0xED, 2, 0x03, 1, 0x00, 0x0D, 1, 0, [0x16, 0x2C]),
    ("VAR_CYCLE_TIME",         0x0C, 0xE7, 1, 0x01, 0, 0x00, 0x00, 0, 0, [0x00]),
    ("VAR_CYCLE_GRAMS",        0x0C, 0xEB, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x00]),
    ("VAR_PROFILE",            0x0C, 0xD3, 1, 0x01, 1, 0x00, 0x04, 0, 0, [0x57]),
    ("VAR_HARDWARE_VERSION",   0x0C, 0xD5, 8, 0x08, 0, 0x00, 0x00, 0, 0, [0x62, 0x69, 0x6F, 0x4D, 0x76, 0x31, 0x2E, 0x30]),
    ("VAR_PROJECT_NAME",       0x0C, 0xD6, 8, 0x08, 0, 0x00, 0x00, 0, 0, [0x66, 0x65, 0x65, 0x64, 0x2D, 0x73, 0x65, 0x6E]),
    ("VAR_AM_TABLE",           0x0D, 0x00, 36, 0x0A, 1, 0x00, 0x50, 1, 0, [0x02, 0x03, 0xE8] * 12),
    ("VAR_PM_TABLE",           0x0D, 0x24, 36, 0x0A, 1, 0x00, 0x74, 1, 0, [0x02, 0x03, 0xE8] * 12),
    ("VAR_ALARMS",             0x0D, 0xBE, 4, 0x04, 0, 0x00, 0x00, 1, 0, [0x00, 0x00, 0x00, 0x00]),
    ("VAR_ALARM_MASK",         0x0D, 0xB5, 4, 0x04, 1, 0x00, 0x23, 1, 0, [0x1F, 0x28, 0x00, 0x76]),
    ("VAR_RTC",                0x0D, 0xD8, 6, 0x06, 1, 0x00, 0x0F, 1, 0, [0x00, 0x00, 0x00, 0x00, 0x01, 0x19]),
    ("VAR_RTC_RESET",          0x0D, 0x50, 6, 0x06, 0, 0x00, 0x00, 0, 0, [0x00, 0x00, 0x00, 0x00, 0x00, 0x00]),
    ("VAR_RESET_COUNT",        0x0D, 0x51, 1, 0x01, 1, 0x00, 0x4B, 0, 0, [0x00]),
    ("VAR_SERVER",             0x0D, 0xD0, 8, 0x08, 1, 0x00, 0x05, 1, 0, [0x00, 0x13, 0xA2, 0x00, 0x41, 0xC1, 0x13, 0xFD]),
    ("VAR_BATTERY",            0x0D, 0x86, 4, 0x04, 0, 0x00, 0x00, 0, 0, [0x00, 0x00, 0x00, 0x00]),
    ("VAR_PANNEL",             0x0D, 0x8D, 4, 0x04, 0, 0x00, 0x00, 0, 0, [0x00, 0x00, 0x00, 0x00]),
    ("VAR_BATTERY_TARA",       0x00, 0x41, 4, 0x04, 1, 0x00, 0x41, 1, 0, [0x00, 0x00, 0x30, 0x00]),
    ("VAR_PANNEL_TARA",        0x00, 0x46, 4, 0x04, 1, 0x00, 0x46, 1, 0, [0x00, 0x00, 0x30, 0x00]),
    ("VAR_LOW_BATTERY",        0x0D, 0x87, 2, 0x03, 1, 0x00, 0xE5, 1, 0, [0x2C, 0xEC]),
    ("VAR_ID_DOSAGE",          0xAB, 0xCD, 2, 0x03, 1, 0x00, 0xA5, 0, 0, [0x00, 0x00]),
    ("VAR_ACCUMULATED_GRAMS",  0x0C, 0x10, 4, 0x05, 1, 0x00, 0xBE, 0, 0, [0x00, 0x00, 0x00, 0x00]),
    ("VAR_DOSED_GRAMS",        0x0C, 0xF1, 2, 0x03, 0, 0x00, 0x00, 1, 0, [0x00, 0x00]),
    ("VAR_TIME_ALARM",         0x00, 0x17, 2, 0x03, 1, 0x00, 0x17, 1, 0, [0x00, 0x1E]),
    ("VAR_PERCENT_MOTOR",      0xFF, 0x02, 1, 0x01, 1, 0x00, 0xC3, 1, 0, [0x64]),
    ("VAR_N_VUELTAS",          0xB0, 0x00, 1, 0x01, 0, 0x00, 0x00, 0, 0, [0x00]),
    ("VAR_T_VUELTAS",          0xB0, 0x01, 4, 0x05, 0, 0x00, 0x00, 0, 0, [0x00, 0x00, 0x00, 0x00]),
    ("VAR_PAN_ID",             0x49, 0x44, 2, 0x03, 0, 0x00, 0x00, 1, 0, [0x00, 0x00]),
    ("VAR_ACS_TYPE",           0xAC, 0x00, 1, 0x01, 1, 0x00, 0xD7, 1, 0, [0x00]),
    ("VAR_AMP_X1",             0xAC, 0x01, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x01]),
    ("VAR_AMP_X2",             0xAC, 0x02, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x02]),
    ("VAR_AMP_X3",             0xAC, 0x03, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x03]),
    ("VAR_PROT_X1",            0xAC, 0x04, 1, 0x01, 1, 0x00, 0xD8, 1, 0, [0x01]),
    ("VAR_PROT_X2",            0xAC, 0x05, 1, 0x01, 1, 0x00, 0xD9, 1, 0, [0x0F]),
    ("VAR_PROT_X3",            0xAC, 0x06, 1, 0x01, 1, 0x00, 0xDA, 1, 0, [0x05]),
    ("VAR_DISCONNECTED",       0xAC, 0x07, 1, 0x01, 1, 0x00, 0xDB, 1, 0, [0x05]),
    ("VAR_EMPTY_HOPPER",       0xAC, 0x08, 2, 0x03, 1, 0x00, 0xDC, 1, 0, [0x01, 0x36]),
    ("VAR_X3_WEAR_PCT",        0xAC, 0x0A, 1, 0x01, 1, 0x00, 0xDD, 1, 0, [0x1E]),
    ("VAR_X3_WEAR_EMA_ALPHA",  0xAC, 0x0B, 1, 0x01, 1, 0x00, 0xDE, 1, 0, [0x02]),
    ("VAR_AMP_MAX_X2",         0xAC, 0x12, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x00]),
    ("VAR_AMP_MIN_X2",         0xAC, 0x13, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x00]),
    ("VAR_AMP_MAX_X3",         0xAC, 0x14, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x00]),
    ("VAR_AMP_MIN_X3",         0xAC, 0x15, 2, 0x03, 0, 0x00, 0x00, 0, 0, [0x00, 0x00]),
    ("VAR_X3_BASELINE_REF",    0xAC, 0x16, 2, 0x03, 1, 0x00, 0xE4, 0, 0, [0x00, 0x00]),
    ("VAR_X3_BASELINE_SLOW",   0xAC, 0x17, 2, 0x03, 1, 0x00, 0xE6, 0, 0, [0x00, 0x00]),
    ("VAR_X3_MOTOR_RESET",     0xAC, 0x18, 1, 0x01, 0, 0x00, 0x00, 1, 0, [0x00]),
    ("VAR_X2_WEAR_PCT",        0xAC, 0x19, 1, 0x01, 1, 0x00, 0xE0, 1, 0, [0x1E]),
    ("VAR_X2_WEAR_EMA_ALPHA",  0xAC, 0x1A, 1, 0x01, 1, 0x00, 0xE1, 1, 0, [0x02]),
    ("VAR_X2_BASELINE_REF",    0xAC, 0x1B, 2, 0x03, 1, 0x00, 0xE2, 0, 0, [0x00, 0x00]),
    ("VAR_X2_BASELINE_SLOW",   0xAC, 0x1C, 2, 0x03, 1, 0x00, 0xE9, 0, 0, [0x00, 0x00]),
    ("VAR_X2_MOTOR_RESET",     0xAC, 0x1D, 1, 0x01, 0, 0x00, 0x00, 1, 0, [0x00]),
    ("VAR_X2_BLADE_AMP_PCT",   0xAC, 0x1E, 1, 0x01, 1, 0x00, 0xEC, 1, 0, [0x19]),
    ("VAR_X2_BLADE_MIN_SWING", 0xAC, 0x1F, 1, 0x01, 1, 0x00, 0xED, 1, 0, [0x0F]),
    ("VAR_X2_BLADE_ACK",       0xAC, 0x20, 1, 0x01, 0, 0x00, 0x00, 1, 0, [0x00]),
    ("VAR_RESET_EEPROM",       0x00, 0x20, 1, 0x01, 1, 0x00, 0x20, 1, 0, [0x00]),
]

ROW = len(ROWS)
MAX_VALUE_SIZE = max(r[3] for r in ROWS)
COL = META_COLS + MAX_VALUE_SIZE


def validate_registry() -> None:
    errors = []
    ids = {}
    eeprom_addrs = {}

    for row in ROWS:
        name, id_msb, id_lsb, size, *_rest, vals = row
        if len(vals) > size:
            errors.append(f"{name}: {len(vals)} value bytes > SIZE {size}")

        var_id = (id_msb << 8) | id_lsb
        if var_id in ids:
            errors.append(
                f"ID duplicado 0x{var_id:04X}: {name} y {ids[var_id]}"
            )
        else:
            ids[var_id] = name

        eeprom_flag = row[5]
        if eeprom_flag:
            eep_msb, eep_lsb = row[6], row[7]
            eep_addr = (eep_msb << 8) | eep_lsb
            if eep_addr in eeprom_addrs:
                errors.append(
                    f"Direccion EEPROM duplicada 0x{eep_addr:04X}: {name} y {eeprom_addrs[eep_addr]}"
                )
            else:
                eeprom_addrs[eep_addr] = name

    if errors:
        print("ERROR: validacion del registro fallo:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        sys.exit(1)


def value_bytes(size, vals, name):
    if len(vals) > size:
        raise ValueError(f"{name}: {len(vals)} value bytes > SIZE {size}")
    out = list(vals)
    if len(out) < size:
        out.extend([0x00] * (size - len(out)))
    return out


def fmt_hex(bytes_list):
    return ", ".join(f"0x{b:02X}" for b in bytes_list)


def fmt_row(name, id_msb, id_lsb, size, fmt, eep, eep_msb, eep_lsb, wr, log_min, vals):
    meta = [id_msb, id_lsb, size, fmt, eep, eep_msb, eep_lsb, wr, log_min]
    value = value_bytes(size, vals, name)
    row = meta + value
    assert len(row) == META_COLS + size, f"{name}: expected {META_COLS + size} init bytes"
    return f"/* {name:<22} */ {{ {fmt_hex(row)} }},"


def header_lines():
    return [
        "/*\n",
        "  GlobalVariables.h - Registro central de variables (generado)\n",
        "  Alimentador Aquasonic — formato compatible con proyecto hub.\n",
        "\n",
        "  NO EDITAR A MANO: usar firmware/tools/gen_var_registry.py\n",
        "\n",
        "  Columnas:\n",
        "    [0-1] ID MSB/LSB\n",
        "    [2]   SIZE (bytes del valor, sin ID)\n",
        "    [3]   FORMAT\n",
        "    [4]   EEPROM (1=persiste en EEPROM interna ATmega)\n",
        "    [5-6] EEPROM ADDRESS MSB/LSB\n",
        "    [7]   WR (1=RW remoto XBee, 0=solo lectura remota)\n",
        "    [8]   LOG_TIME (minutos, 0=sin log periodico)\n",
        f"    [9 .. 9+SIZE-1] VALUE (exactamente SIZE bytes; resto de COL se inicializa en 0)\n",
        "\n",
        "*/\n",
        "#ifndef AQUASONIC_GLOBAL_VARIABLES_H_\n",
        "#define AQUASONIC_GLOBAL_VARIABLES_H_\n",
        "\n",
        "#include <Arduino.h>\n",
        "#include <stdint.h>\n",
        "\n",
        "/* Indices de fila */\n",
    ]


def common_defines_lines():
    lines = []
    for i, (name, *_) in enumerate(ROWS):
        lines.append(f"#define {name:<22} {i}\n")
    lines.extend([
        "\n",
        "/* Columnas (igual que hub) */\n",
        "#define VAR_ID              0\n",
        "#define VAR_SIZE            2\n",
        "#define VAR_FORMAT          3\n",
        "#define VAR_EEPROM          4\n",
        "#define VAR_EEPROM_ADDRESS  5\n",
        "#define VAR_WR              7\n",
        "#define VAR_LOG_TIME        8\n",
        f"#define VAR_VALUE           {VAR_VALUE}\n",
        "\n",
        f"#define ROW {ROW}\n",
        f"#define COL {COL}   /* {META_COLS} meta + hasta {MAX_VALUE_SIZE} bytes VALUE por fila */\n",
        "\n",
        "extern uint8_t transferVar[ROW][COL];\n",
        "\n",
        "#endif /* AQUASONIC_GLOBAL_VARIABLES_H_ */\n",
    ])
    return lines


def matrix_lines():
    lines = [
        "/* Matriz de variables — generada por gen_var_registry.py */\n",
        "uint8_t transferVar[ROW][COL] =\n",
        "{\n",
        "//       |     ID     | SIZE | FMT | EEP | EEP ADDR | WR | LOG | VALUE (SIZE bytes)...\n",
    ]
    for row in ROWS:
        lines.append("  " + fmt_row(*row) + "\n")
    lines.append("};\n")
    return lines


def main():
    validate_registry()

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    h_body = header_lines() + common_defines_lines()
    OUT_H.write_text("".join(h_body), encoding="utf-8")

    cpp_body = [
        '#include "variables/GlobalVariables.h"\n',
        "\n",
    ] + matrix_lines()
    OUT_CPP.write_text("".join(cpp_body), encoding="utf-8")

    print(f"Wrote {OUT_H} and {OUT_CPP} ({ROW} vars, COL={COL})")


if __name__ == "__main__":
    main()
