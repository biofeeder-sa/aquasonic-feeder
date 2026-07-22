#!/usr/bin/env python3
"""Split firmware.ino and vars.h into modular .h/.cpp files."""

from pathlib import Path

FIRMWARE = Path(__file__).resolve().parent.parent

# (filename, start_line_1indexed, end_line_1indexed) — inclusive, from original firmware.ino
MODULE_SECTIONS = {
    "actuators.cpp": [(20, 22), (3003, 3030), (3253, 3278)],
    "rtc.cpp": [(538, 626), (943, 992), (997, 1066), (3196, 3251), (3280, 3297)],
    "dosage.cpp": [(628, 938), (3559, 3621)],
    "eeprom_store.cpp": [(1071, 1186), (1191, 1257), (1316, 1404)],
    "sensors.cpp": [(1262, 1314), (2973, 3001), (3035, 3053)],
    "xbee.cpp": [
        (24, 51),
        (1413, 1616),
        (1618, 1625),
        (1627, 2618),
        (2620, 2971),
        (3055, 3075),
        (3077, 3085),
        (3087, 3194),
        (3623, 3629),
    ],
}

MODULE_HEADERS = {
    "actuators.h": ["void activarPWM(int valorPWM, int output);", "void InitLed(void);",
                    "void motorStarting(int _delay, int _interval, int _motor, uint8_t percent);"],
    "rtc.h": ["void Internal_Clock(void);", "void Print_Hour(void);",
              "uint8_t BCD_TO_HEXA(uint8_t num);", "uint8_t HEXA_TO_BCD(uint8_t byte);",
              "void ChangeGramsAndTime(void);", "void forTimeAlam(void);"],
    "dosage.h": ["void Dosage_FeedRate(void);", "void calibration();"],
    "eeprom_store.h": ["void Start_EEPROM(void);",
                       "void Write_EEPROM_Gral(uint32_t valor, uint8_t bytes, uint32_t init_direct);",
                       "void Update_EEPROM(void);"],
    "sensors.h": ["void Sense_Voltajes(void);", "unsigned long readCount();"],
    "xbee.h": [
        "void create_frame(unsigned char frame[], unsigned char data[], unsigned char data_size, unsigned char frame_id);",
        "void set_checksum(unsigned char frame[], int size);",
        "unsigned char checksum(unsigned char frame[], int size);",
        "void print_frame(const char *header, unsigned char frame[], unsigned char data_size, const char *footer);",
        "void send_frame(unsigned char frame[], int size);",
        "void printDigits(int digits);",
        "void create_init_broadcast(unsigned char frame[], unsigned char data[], unsigned char data_size);",
        "void create_data_broadcast();",
        "void broadcast();",
        "void xbee_communication();",
        "void create_init_Alarms(unsigned char frame[], unsigned char data[], unsigned char data_size);",
        "void create_data_Alarms(void);",
        "void Alarms(void);",
        "void Send_Sequence_Alarms(void);",
        "void Send_Sequence_Broadcast(void);",
        "void NewServer(void);",
        "void serialEventRun(void);",
        "void create_init_unicast(unsigned char frame[], unsigned char data[], unsigned char data_size);",
        "void create_data_unicast();",
        "void unicast();",
        "void cleanAlarms();",
    ],
}

CPP_INCLUDES = """\
#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>

#include "vars.h"
#include "lib/ACS/ACS.h"

"""


def read_ino_lines():
    path = FIRMWARE / "firmware.ino"
    return path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)


def extract_sections(lines, ranges):
    chunks = []
    for start, end in ranges:
        chunks.extend(lines[start - 1 : end])
    return chunks


def write_module_cpp(name, ranges):
    lines = read_ino_lines()
    body = extract_sections(lines, ranges)
    content = CPP_INCLUDES + "\n" + "".join(body)
    (FIRMWARE / name).write_text(content, encoding="utf-8")
    print(f"Wrote {name} ({len(body)} lines)")


def write_module_headers():
    for name, decls in MODULE_HEADERS.items():
        guard = name.upper().replace(".", "_")
        content = f"#ifndef {guard}\n#define {guard}\n\n#include <stdint.h>\n\n"
        content += "\n".join(decls) + "\n\n#endif\n"
        (FIRMWARE / name).write_text(content, encoding="utf-8")
        print(f"Wrote {name}")


def split_vars_h():
    text = (FIRMWARE / "vars.h").read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines(keepends=True)

    # config.h: logical states + pins + xbee protocol constants (lines 6-30, 340-374 approx)
    config_parts = []
    for i, line in enumerate(lines):
        n = i + 1
        if 6 <= n <= 30:
            config_parts.append(line)
        elif 340 <= n <= 374:
            config_parts.append(line)

    config = """\
#ifndef AQUASONIC_CONFIG_H_
#define AQUASONIC_CONFIG_H_

#include <stdint.h>

"""
    config += "".join(config_parts)
    config += "\n#endif\n"
    (FIRMWARE / "config.h").write_text(config, encoding="utf-8")

    # types.h: struct definitions (lines 190-227)
    types = """\
#ifndef AQUASONIC_TYPES_H_
#define AQUASONIC_TYPES_H_

"""
    types += "".join(lines[189:227])
    types += "\n#endif\n"
    (FIRMWARE / "types.h").write_text(types, encoding="utf-8")

    # globals: variable blocks (skip prototypes 496-541, skip struct block already in types)
    globals_header = """\
#ifndef AQUASONIC_GLOBALS_H_
#define AQUASONIC_GLOBALS_H_

#include <stdint.h>
#include "config.h"
#include "types.h"

"""
    globals_cpp = """\
#include "globals.h"

"""
    # Lines 32-36 macros, 38-189 vars, 228-494 vars (skip 190-227 structs)
    for i, line in enumerate(lines):
        n = i + 1
        if n == 496:
            break
        if 190 <= n <= 227:
            continue
        if n < 32:
            continue
        stripped = line.strip()
        if not stripped or stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//"):
            if 32 <= n <= 36:
                globals_header += line
            continue
        if "typedef struct" in line:
            continue
        # Convert definition to extern in header, keep definition in cpp
        if line.strip().startswith("#define"):
            globals_header += line
            continue
        if "=" in line and not line.strip().startswith("//"):
            # variable definition
            decl = line.split("=")[0].rstrip() + ";\n"
            globals_header += "extern " + decl.lstrip()
            globals_cpp += line
        elif line.strip().endswith(";") and not line.strip().startswith("#"):
            globals_header += "extern " + line.lstrip()
            globals_cpp += line

    globals_header += """
/* Perfiles de orden/tipo de dosificacion (requieren transferProfile) */
#define Events_ArduinoUNO     ((transferProfile[2] == 0x57))
#define Table_ArduinoUNO      ((transferProfile[2] == 0x07))
#define FeedRate_Dosage       ((transferProfile[2] == 0x07) || (transferProfile[2] == 0x57))

#endif
"""
    (FIRMWARE / "globals.h").write_text(globals_header, encoding="utf-8")
    (FIRMWARE / "globals.cpp").write_text(globals_cpp, encoding="utf-8")
    print("Wrote config.h, types.h, globals.h, globals.cpp")


def write_vars_shim():
    content = """\
#ifndef PROVIC_UNO_VARIABLES_H_
#define PROVIC_UNO_VARIABLES_H_

#include "config.h"
#include "types.h"
#include "globals.h"
#include "actuators.h"
#include "rtc.h"
#include "dosage.h"
#include "eeprom_store.h"
#include "sensors.h"
#include "xbee.h"

#endif
"""
    (FIRMWARE / "vars.h").write_text(content, encoding="utf-8")
    print("Wrote vars.h shim")


def write_firmware_ino():
    lines = read_ino_lines()
    setup_loop = extract_sections(lines, [(53, 533)])
    content = """\
// ALIMENTADOR Aquasonic (ATmega 2560)
// Base: biomatic-feeder v00.00.27 — proyecto modularizado

#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "vars.h"
#include "lib/ACS/ACS.h"
#include <TimeLib.h>
#include <EEPROM.h>

"""
    content += "".join(setup_loop)
    (FIRMWARE / "firmware.ino").write_text(content, encoding="utf-8")
    print("Wrote firmware.ino (setup + loop only)")


def main():
    split_vars_h()
    write_module_headers()
    for name, ranges in MODULE_SECTIONS.items():
        write_module_cpp(name, ranges)
    write_vars_shim()
    write_firmware_ino()


if __name__ == "__main__":
    main()
