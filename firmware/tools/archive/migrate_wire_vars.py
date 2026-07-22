#!/usr/bin/env python3
"""Replace legacy transfer* wire arrays with VAR_WIRE_BYTE(row, idx) macros."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# (legacy_name, row_constant) — longest names first
WIRE_MAP = [
    ("transferAccumulatedGrams", "VAR_ACCUMULATED_GRAMS"),
    ("transferWeightReference", "VAR_WEIGHT_REF_RAW"),
    ("transferDifferenceTara", "VAR_DIFF_TARA"),
    ("transferWeightTara", "VAR_WEIGHT_TARA"),
    ("transferResetEEPROM", "VAR_RESET_EEPROM"),
    ("transferDosedgrams", "VAR_DOSED_GRAMS"),
    ("transfertimeAlarm", "VAR_TIME_ALARM"),
    ("transferPercentMotor", "VAR_PERCENT_MOTOR"),
    ("transferCycleGrams", "VAR_CYCLE_GRAMS"),
    ("transferTaraBattery", "VAR_BATTERY_TARA"),
    ("transferTaraPannel", "VAR_PANNEL_TARA"),
    ("transferLowBattery", "VAR_LOW_BATTERY"),
    ("transferUnderWeight", "VAR_UNDER_WEIGHT"),
    ("transferalarmMask", "VAR_ALARM_MASK"),
    ("transferSetBytes", "VAR_SETTING_BYTES"),
    ("transferFeedRate", "VAR_FEED_RATE"),
    ("transferCycleTime", "VAR_CYCLE_TIME"),
    ("transferTableAM", "VAR_AM_TABLE"),
    ("transferTablePM", "VAR_PM_TABLE"),
    ("transferProfile", "VAR_PROFILE"),
    ("transferServer", "VAR_SERVER"),
    ("transferBattery", "VAR_BATTERY"),
    ("transferPannel", "VAR_PANNEL"),
    ("transferReference", "VAR_REFERENCE_KG"),
    ("transferAlarms", "VAR_ALARMS"),
    ("transferRTCReset", "VAR_RTC_RESET"),
    ("transferResetCount", "VAR_RESET_COUNT"),
    ("transferNvueltas", "VAR_N_VUELTAS"),
    ("transferTvueltas", "VAR_T_VUELTAS"),
    ("transferWeight", "VAR_WEIGHT"),
    ("typeSensorACS", "VAR_ACS_TYPE"),
    ("disconnectedXX", "VAR_DISCONNECTED"),
    ("emptyHopper", "VAR_EMPTY_HOPPER"),
    ("ampWarningX2", "VAR_AMP_WARN_X2"),
    ("ampWarningX3", "VAR_AMP_WARN_X3"),
    ("protectionX1", "VAR_PROT_X1"),
    ("protectionX2", "VAR_PROT_X2"),
    ("protectionX3", "VAR_PROT_X3"),
    ("ampMaxX2", "VAR_AMP_MAX_X2"),
    ("ampMinX2", "VAR_AMP_MIN_X2"),
    ("ampMaxX3", "VAR_AMP_MAX_X3"),
    ("ampMinX3", "VAR_AMP_MIN_X3"),
    ("transferlat", "VAR_LATITUDE"),
    ("transferlng", "VAR_LONGITUDE"),
    ("transferRTC", "VAR_RTC"),
    ("transferID", "VAR_ID_DOSAGE"),
    ("panID", "VAR_PAN_ID"),
    ("AmpX1", "VAR_AMP_X1"),
    ("AmpX2", "VAR_AMP_X2"),
    ("AmpX3", "VAR_AMP_X3"),
]

SKIP = {"tools", "registry", "lib", "migrate_wire_vars.py", "gen_var_registry.py", "modularize.py"}


def transform(text: str) -> str:
    for name, row in WIRE_MAP:
        text = re.sub(
            rf"sizeof\(\s*{re.escape(name)}\s*\)\s*/\s*sizeof\(\s*{re.escape(name)}\s*\[\s*0\s*\]\s*\)",
            f"VAR_WIRE_LEN({row})",
            text,
        )
        text = re.sub(
            rf"{re.escape(name)}\s*\[",
            f"VAR_WIRE_BYTE({row}, ",
            text,
        )
        text = re.sub(
            rf"VAR_WIRE_BYTE\({row},\s*([^\]\)]+)\]",
            rf"VAR_WIRE_BYTE({row}, \1)",
            text,
        )
    # software/hardware/project: solo ID (2 bytes)
    for name, row in [("software", "VAR_SOFTWARE_VERSION"), ("hardware", "VAR_HARDWARE_VERSION"), ("project", "VAR_PROJECT_NAME")]:
        text = re.sub(rf"{re.escape(name)}\s*\[", f"VAR_WIRE_BYTE({row}, ", text)
    return text


def main():
    for path in sorted(ROOT.rglob("*")):
        if path.suffix not in {".cpp", ".h", ".ino"}:
            continue
        if any(part in SKIP for part in path.parts):
            continue
        if path.name in {"globals.cpp", "globals.h"}:
            continue
        original = path.read_text(encoding="utf-8")
        updated = transform(original)
        if updated != original:
            path.write_text(updated, encoding="utf-8")
            print(f"updated: {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
