# Native SmallTV SDC2 OTA result

Tested commit: 7e06a122ed219b965c6b8306a440c98b295252ab.

- Source engine: original src/features/theme/ThemeEngine.cpp, compiled unchanged.
- ESP-IDF 5.5.1 native SDC2 adapter build: PASS.
- Actual SmallTV parser/compositor: 288 configuration, bounds and update cases, AddressSanitizer/UBSan: PASS.
- Settings validation/roundtrip and coordinate URL formatting: PASS.
- Native web JavaScript syntax: PASS.
- Public Open-Meteo HTTP endpoint: 200 with temperature and humidity, PASS.
- App-only OTA image: SmallTV_SDC2_NATIVE_OTA.bin, 685008 bytes.
- Existing app slot: 786432 bytes; 101424 bytes remain.
- ESP32-C2 image/header, DIO/60 MHz, revisions 100..299, segment checksum and SHA-256: PASS.
- SHA-256: 6ed7258900c01311cdd97d8e08402f1826aa225a77db84d6f87a588c9afea160.
- Build: https://github.com/pop7523/smalltv-mod/actions/runs/44965691053
- Download: https://github.com/pop7523/smalltv-mod/actions/runs/44965691053/artifacts/14730952977

Unzip the artifact and upload SmallTV_SDC2_NATIVE_OTA.bin through the current firmware's Firmware upload control. This app-only image preserves the partition table and filesystem; uploading replaces the other application slot. Do not use the USB factory image or USB new-layout OTA image.

This is the native clock/weather-focused SmallTV engine port documented in README.md. It does not include the entire original Arduino application or theme asset package uploads. No Stock Clock application modules are compiled.

No physical device write has been performed. Device access from the execution environment was unavailable. The physical LCD, Wi-Fi reconnection, live device weather and reboot after OTA remain unverified.
