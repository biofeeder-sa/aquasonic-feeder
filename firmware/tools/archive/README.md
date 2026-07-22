# Scripts de migración (archivados)

Scripts one-shot usados durante la modularización del firmware. **No forman parte del flujo de build.**

| Script | Uso original |
|--------|----------------|
| `modularize.py` | Partió el `.ino` monolítico en módulos `src/` |
| `migrate_wire_vars.py` | Reemplazó arrays `transfer*` por `VAR_WIRE_BYTE()` |
| `fix_wire_brackets.py` | Corrección de sintaxis tras la migración wire |

El registro activo de variables está en `../gen_var_registry.py` (invocado por `scripts/validate_registry.ps1`).
