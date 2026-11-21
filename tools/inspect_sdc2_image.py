"""Read-only ESP32-C2 OTA image checks; no network or device writes.

Format follows espressif/esptool bin_image.py (ESP32FirmwareImage).
A passing report does not prove the physical board or filesystem works.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def inspect(path):
    raw = Path(path).read_bytes()
    if not 24 <= len(raw) <= 0xC0000:
        raise ValueError(f"Image size {len(raw)} is outside the stock OTA slot")
    magic, count, mode, speed_size, entry = struct.unpack_from("<BBBBI", raw)
    if magic != 0xE9 or not 1 <= count <= 16:
        raise ValueError("Invalid ESP application header")
    fields = struct.unpack_from("<BBBBHBHHBBBBB", raw, 8)
    chip, minimum, maximum, digest = fields[4], fields[6], fields[7], fields[-1]
    if chip != 12:
        raise ValueError(f"Expected ESP32-C2 chip ID 12, got {chip}")
    if mode != 2 or (speed_size & 15) != 15:
        raise ValueError("Expected DIO / 60 MHz flash settings")
    if minimum != 100 or maximum != 299:
        raise ValueError(f"Unverified chip revision range {minimum}..{maximum}")
    if digest != 1:
        raise ValueError("Missing appended image SHA-256")
    cursor, checksum, description = 24, 0xEF, None
    for index in range(count):
        if cursor + 8 > len(raw):
            raise ValueError("Truncated segment header")
        address, size = struct.unpack_from("<II", raw, cursor)
        cursor += 8
        end = cursor + size
        if end > len(raw):
            raise ValueError("Truncated segment data")
        data = raw[cursor:end]
        if index == 0:
            if len(data) < 80 or struct.unpack_from("<I", data)[0] != 0xABCD5432:
                raise ValueError("Not an app-only OTA image: missing application descriptor")
            description = {
                "version": data[16:48].split(b"\0")[0].decode("utf-8", "replace"),
                "project": data[48:80].split(b"\0")[0].decode("utf-8", "replace"),
            }
        for byte in data:
            checksum ^= byte
        cursor = end
    checksum_at = ((cursor + 16) // 16) * 16 - 1
    digest_at = checksum_at + 1
    if digest_at + 32 > len(raw):
        raise ValueError("Truncated checksum/digest")
    if raw[checksum_at] != checksum:
        raise ValueError("Segment checksum mismatch")
    if hashlib.sha256(raw[:digest_at]).digest() != raw[digest_at:digest_at + 32]:
        raise ValueError("Image SHA-256 mismatch")
    if any(byte not in (0, 255) for byte in raw[digest_at + 32:]):
        raise ValueError("Unexpected content after image (merged/factory file?)")
    return {"image_checks": "PASS", "bytes": len(raw), "chip_id": chip,
            "revision_min": minimum, "revision_max": maximum,
            "sha256": hashlib.sha256(raw).hexdigest(), **description,
            "hardware_validated": False, "device_written": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(inspect(args.image), indent=2))
    except (ValueError, OSError, struct.error) as exc:
        raise SystemExit(f"IMAGE REJECTED: {exc}")
