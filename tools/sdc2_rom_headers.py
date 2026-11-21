"""Expose IDF's matching ROM-mbedTLS port header to the Arduino child build.

No header is replaced: the installed framework-espidf is the source of truth.
The pioarduino generated Arduino include set omits this directory for ROM crypto.
"""
from pathlib import Path

Import("env")

idf = env.PioPlatform().get_package_dir("framework-espidf")
if idf:
    rom = Path(idf) / "components/mbedtls/port/mbedtls_rom"
    if (rom / "threading_alt.h").is_file():
        env.AppendUnique(CPPPATH=[str(rom)])
