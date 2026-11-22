# SDC2 GIF OTA result

Tested commit: f889b46b7e23cbb8ed1330ddae5a252d51519783.

- Firmware: 3.11.0-sdc2-gif.1, SmallTV_SDC2_NATIVE_OTA.bin.
- App-only image: 729600 bytes; existing 786432-byte OTA slot, 56832 bytes free.
- ESP32-C2 / DIO 60 MHz / revisions 100..299, segment checksum and SHA-256: PASS.
- SHA-256: 853e52dbb75b73b4d881f93048c011be01b70ce81af167c5fd37b9f47bfb6291.
- Actual SmallTV engine: 1440 configuration/bounds/update cases, ASan/UBSan: PASS.
- Real GIF disposal 1/2/3 conversion versus independent Pillow RGBA frames: PASS.
- Native animation decoder, alpha/clipping, malformed/truncated files: PASS under ASan/UBSan.
- Wrong dimensions, frame count and converted-size limits: PASS.
- Browser JavaScript syntax: PASS.
- Open-Meteo current temperature/humidity HTTP probe: PASS.

Build: https://github.com/pop7523/smalltv-mod/actions/runs/44967790721

Download: https://github.com/pop7523/smalltv-mod/actions/runs/44967790721/artifacts/14732601428

Upload the app-only .bin through the current device's Firmware upload control.
After reboot, open the 80x80 GIF section, upload up to three .gif files, and save
the desired display mode. Converted animations persist in the existing data
partition. Images must be exactly 80x80; 1..300 frames, up to 2 MiB source and
384 KiB after conversion per slot. Three GIFs can be displayed side by side.

No partition table or NVS erase is part of OTA. If the existing image filesystem
cannot be mounted, its initialization requires an explicit browser confirmation
and removes the data partition's old image files. Mounting does not auto-format.

No device write or physical-device playback/filesystem validation was performed.
