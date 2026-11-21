# SDC2 Desk Clock build result

Tested commit: `98513c6dafc5434533d739d55cba8400c36e812b`.

- Native ESP-IDF 5.5.1 build: PASS.
- Renderer: 144 full-frame versus 16-row strip comparisons with AddressSanitizer and UBSan: PASS.
- Web JavaScript syntax check: PASS.
- App-only binary: `SDC2_DeskClock_v1.1.0-desk.1_OTA.bin`, 695536 bytes.
- Actual SDC2 OTA slot: 786432 bytes, 90896 bytes free.
- ESP32-C2 ID 12, revisions 100..299, DIO 60 MHz, segment checksum and appended SHA-256: PASS.
- SHA-256: `6d6f8cab96de6140c54c41d5aec101e479bd17f08bfd314ea3594eaae2807453`.
- Build and artifact: https://github.com/pop7523/smalltv-mod/actions/runs/44960206620
- Download: https://github.com/pop7523/smalltv-mod/actions/runs/44960206620/artifacts/14729397096

No device write was performed. HTTP connection to 192.168.35.75:80 failed from this execution environment. Physical display, Wi-Fi reconnection and live weather remain unverified on the user's device.

To install from the running Stock Clock web interface: unzip the artifact, open the device's web page, select the app-only OTA .bin in Firmware and upload. Uploading replaces the other app slot. This is a custom Stock Clock based build, not the manufacturer's original firmware.
