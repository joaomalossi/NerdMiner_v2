#!/usr/bin/env python3
"""Gera src/dashboard/dashboard_page.h a partir de src/dashboard/web/index.html.

O firmware serve a pagina gzipada direto da flash (PROGMEM). O header gerado
e commitado no repo — rode este script sempre que editar o index.html:

    python3 scripts/gen_dashboard_page.py
"""
import gzip
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "dashboard" / "web" / "index.html"
OUT = ROOT / "src" / "dashboard" / "dashboard_page.h"

raw = SRC.read_bytes()
gz = gzip.compress(raw, mtime=0)  # mtime=0: saida deterministica

lines = []
for i in range(0, len(gz), 16):
    chunk = gz[i:i + 16]
    lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")

OUT.write_text(
    "// Gerado por scripts/gen_dashboard_page.py a partir de web/index.html — NAO editar na mao.\n"
    "#ifndef DASHBOARD_PAGE_H\n"
    "#define DASHBOARD_PAGE_H\n\n"
    "#include <pgmspace.h>\n\n"
    f"#define DASHBOARD_PAGE_GZ_LEN {len(gz)}\n\n"
    "static const uint8_t DASHBOARD_PAGE_GZ[] PROGMEM = {\n"
    + "\n".join(lines)
    + "\n};\n\n#endif // DASHBOARD_PAGE_H\n"
)
print(f"{SRC.name}: {len(raw)} bytes -> {OUT.name}: {len(gz)} bytes gzip")
