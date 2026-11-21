# SmallTV for JUZIPi SDC2: experimental source checkpoint

Status: remote full-feature application compilation/linking completed, but the
image exceeds the stock slot. NOT installable, NOT flashed.

## Remote build evidence

Fork: https://github.com/pop7523/smalltv-mod/tree/sdc2-build

- Run 44930819388 built the custom IDF libraries, then exposed a recursive
  child-build config error. The SDC2 profile is now included by platformio.ini.
- Run 44931301162 compiled application sources but exposed a missing ROM
  mbedTLS port include. The build now uses the OFFICIAL matching installed IDF
  port/mbedtls_rom directory, not a replacement mutex implementation.
- Run 44932032897 completed full application compilation and linking. Its
  program size was 1403508 bytes; IDF reported total image sections of 1416100
  bytes before binary padding. The 786432-byte stock app limit rejected it.
- Actual framework resolved by the pinned pioarduino platform is IDF 5.5.5,
  Arduino 3.3.11. Compatibility with the stock 5.5.1 bootloader has NOT been
  demonstrated on hardware.
- Run 44932715207 also completed application compilation and linking with
  ticker, radar, usage-meter and MQTT disabled. Program size: 1375948 bytes;
  IDF image-section total: 1388532 bytes before binary padding. This exceeds
  the stock slot by 589516 bytes. Module removal did NOT make stock OTA viable.
- inspect_sdc2_image.py passed against the known 689888-byte SDC2 Stock Clock
  app and rejected wrong-chip, corrupt-data, corrupt-digest, oversize and
  truncated files. This tests the validator, not a new firmware or the device.

No partition table has been changed. No replacement firmware has been sent
to the device. The earlier host-only checks are not evidence of board operation.

Conclusion: this Arduino-based port is not installable within the stock OTA
layout. A viable next step requires either a verified, backed-up flash-layout
migration (preferably with UART recovery available) or a substantially different
implementation that fits 768 KiB. No migration code has been implemented or
tested, and no uploadable firmware is being claimed.

## Latest test attempt

- `python tools/check_sdc2_port.py` exercises the actual size post-action at
  786431, 786432, 786433 and 1493184 bytes, checks the stock partition bounds
  and upload blocker. These are host checks, not firmware/hardware tests.
- Official v3.11.0 C2 app asset: 1493184 bytes (GitHub release API), which
  exceeds the stock 786432-byte slot by 706752 bytes (47.3% of the app).
- Explicit PyPI installation and DNS lookup both failed on this executor.
  The clock at 192.168.35.75 also could not be reached. No image was compiled.
- On a network-enabled host, run `Build-SDC2.cmd` (or
  `python tools/build_sdc2.py`) to collect the host, installation and compiler
  logs in `build-logs/`. It never uploads firmware, even on build success.

## Provenance

Upstream build-source snapshot: giovi321/smalltv-mod at
e66475e9376852d3d090d7d27f9bafc3258f200d (3.11.0).
112 build-related text files were retrieved through the GitHub connector because
normal git/network access failed. This is not a full upstream clone/history;
docs, CI, examples and upstream tests are not included.

SDC2 reference: https://github.com/yanirta/sdc2-stock-clock
The reference README describes the ACTUAL stock slots as 0xc0000 bytes each.
Its partitions.csv instead has larger build-time-only slots: do NOT copy that
file as evidence of the device's real flash layout.

## Prepared changes

- A separate SDC2 display header: CLK1, MOSI2, DC0, RST3, BL5, CS grounded.
- Separate build candidate for the reported stock layout (768 KiB per slot).
- 26 MHz crystal request and a compile-time sdkconfig check.
- WireGuard and upstream GitHub self-update disabled for this candidate so an
  update cannot silently replace it with the incompatible generic C2 image.
- No automatic LittleFS format when the stock filesystem cannot be mounted.
- Build-size gate at 786432 bytes and a blocked PlatformIO upload command.

## Blocking work before any installation

1. Install the pinned PlatformIO/toolchain and build. No PlatformIO or cached
   ESP-IDF installation was available; pip installation could not resolve a
   package and direct git access failed in this session.
2. Confirm the pioarduino 55.03.311 toolchain actually rebuilds the C2 libraries
   for 26 MHz, rather than retaining a prebuilt 40 MHz configuration. Check the
   complete generated sdkconfig and the final image header/revision bounds.
   The working reference uses ESP-IDF 5.5.1 and C2 rev 1.0-2.99; compatibility
   with the different Arduino/IDF release is not yet established.
3. Measure image size. Generic C2 images are around 1.5 MB; the stock SDC2 slot
   is 768 KiB. Removing WireGuard alone is NOT expected to close this gap.
   Keep theme support (the user's goal); investigate IDF/ROM/TLS reductions
   before claiming a usable build. No partition rewrite is implemented.
4. Implement an explicit filesystem migration/backup path. Current settings
   cannot persist if the existing stock filesystem is not LittleFS. Never
   format it without preserving stock photos/settings and a recovery path.
5. Implement and test return-to-stock/recovery before a first OTA trial. This
   checkpoint has no SDC2-specific automatic boot fallback yet.
6. Verify the physical unit/stock partition table and crystal, then test display,
   network, settings, themes and manual OTA. No device writes were attempted.

## Candidate build (not a flashing instruction)

`.github/workflows/sdc2-build.yml` provides a network-enabled GitHub Actions
build route (manual dispatch or a push to `sdc2-build`). It pins BOTH PlatformIO
cores, including the pioarduino child launched during custom IDF rebuilds,
matching upstream's cold-build workaround. It has not been run in this session:
no user-owned destination repository has been selected. It publishes diagnostic
logs/maps only, not an installable release or a factory image.

```
python -m platformio run -c platformio.sdc2.ini -e smalltv_sdc2
```

Even a passing size gate does NOT certify bootloader, storage or hardware
compatibility. Do not rename a generic C2 binary to SDC2 and upload it.
