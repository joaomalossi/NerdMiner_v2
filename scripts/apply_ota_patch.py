#!/usr/bin/env python3
"""
Aplica o patch de auto-update (OTA) sobre um checkout limpo do NerdMiner_v2
upstream. Usado pela Action de rebuild automatico (.github/workflows/ota-rebuild.yml).

Falha alto (exit 1) se algum ponto de ancoragem esperado nao existir no
upstream, em vez de aplicar o patch parcialmente e publicar um build quebrado.
"""
import shutil
import sys
from pathlib import Path

def die(msg):
    print(f"ERRO: {msg}", file=sys.stderr)
    sys.exit(1)

def patch_main_cpp(path: Path):
    text = path.read_text()

    include_anchor = '#include "monitor.h"'
    if include_anchor not in text:
        die(f"ancora de include nao encontrada em {path}")
    if '#include "otaUpdater.h"' not in text:
        text = text.replace(include_anchor, include_anchor + '\n#include "otaUpdater.h"', 1)

    setup_anchor = "setup_monitor();"
    if setup_anchor not in text:
        die(f"ancora de setup_monitor() nao encontrada em {path}")
    if "setup_ota_updater();" not in text:
        text = text.replace(
            setup_anchor,
            setup_anchor + "\n\n  /******** OTA AUTO-UPDATE SETUP *****/\n  setup_ota_updater();",
            1,
        )

    path.write_text(text)
    print(f"OK: {path} patched")

def patch_platformio_ini(path: Path):
    text = path.read_text()

    anchor = "-D DEVKITV1=1"
    if anchor not in text:
        die(f"ancora '{anchor}' nao encontrada em {path}")
    if "OTA_ASSET_NAME" not in text:
        text = text.replace(
            anchor,
            anchor + '\n\t-D OTA_ASSET_NAME=\\"ESP32-devKitv1_firmware.bin\\"',
            1,
        )

    path.write_text(text)
    print(f"OK: {path} patched")

def main():
    if len(sys.argv) != 3:
        die("uso: apply_ota_patch.py <dir_upstream_limpo> <dir_deste_repo>")

    upstream_dir = Path(sys.argv[1])
    this_repo_dir = Path(sys.argv[2])

    main_cpp = upstream_dir / "src" / "NerdMinerV2.ino.cpp"
    platformio_ini = upstream_dir / "platformio.ini"

    if not main_cpp.exists():
        die(f"{main_cpp} nao existe - upstream mudou de estrutura?")
    if not platformio_ini.exists():
        die(f"{platformio_ini} nao existe - upstream mudou de estrutura?")

    shutil.copy(this_repo_dir / "src" / "otaUpdater.h", upstream_dir / "src" / "otaUpdater.h")
    shutil.copy(this_repo_dir / "src" / "otaUpdater.cpp", upstream_dir / "src" / "otaUpdater.cpp")
    print("OK: otaUpdater.h/.cpp copiados")

    patch_main_cpp(main_cpp)
    patch_platformio_ini(platformio_ini)

if __name__ == "__main__":
    main()
