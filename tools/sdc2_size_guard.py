"""PlatformIO post-build gate; this does NOT certify a binary for flashing."""
from pathlib import Path

Import("env")


def check_size(source, target, env):
    image = Path(env.subst("$BUILD_DIR")) / "firmware.bin"
    limit = 0xC0000
    size = image.stat().st_size
    if size > limit:
        raise RuntimeError(f"SDC2 stock OTA slot is {limit} bytes; image is {size}")
    print(f"SDC2 size gate: {size}/{limit} bytes. NOT hardware validated.")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)
