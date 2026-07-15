#!/usr/bin/env python3
"""
Aplica os patches deste fork sobre um checkout limpo do NerdMiner_v2 upstream.
Usado pela Action de rebuild automatico (.github/workflows/ota-rebuild.yml).

1. Patch de auto-update (OTA): copia otaUpdater.h/.cpp e edita ancoras.
2. Patch de PPLNS (patches/pplns-large-notify.patch): buffers do stratum eram
   pequenos demais para os mining.notify de ~14KB da porta PPLNS da public-pool
   (coinbase paga todos os participantes) - sem ele o miner fica em 0 H/s
   silenciosamente nessa porta.

Falha alto (exit 1) se algum ponto de ancoragem esperado nao existir no
upstream, em vez de aplicar o patch parcialmente e publicar um build quebrado.
"""
import shutil
import subprocess
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

def apply_pplns_patch(upstream_dir: Path, this_repo_dir: Path):
    patch = (this_repo_dir / "patches" / "pplns-large-notify.patch").resolve()
    if not patch.exists():
        die(f"{patch} nao existe")

    git_apply = ["git", "-C", str(upstream_dir), "apply"]

    if subprocess.run(git_apply + ["--reverse", "--check", str(patch)],
                      capture_output=True).returncode == 0:
        print("OK: patch PPLNS ja aplicado, pulando")
        return

    check = subprocess.run(git_apply + ["--check", str(patch)],
                           capture_output=True, text=True)
    if check.returncode != 0:
        die(f"patch PPLNS nao aplica mais sobre o upstream (codigo mudou?):\n{check.stderr}")

    subprocess.run(git_apply + [str(patch)], check=True)
    print("OK: patch PPLNS (stratum/utils) aplicado")

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
    apply_pplns_patch(upstream_dir, this_repo_dir)

if __name__ == "__main__":
    main()
