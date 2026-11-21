# SDC2 Desk Clock

Based on https://github.com/yanirta/sdc2-stock-clock at e46003bd6fa3999daa9924067b93cd39b4dad4fd.
The original source and provenance are retained in this directory. This subtree is not covered by the parent project's license declaration.

Adds a persistent large clock and current weather, three layouts, background/text/accent colors, seconds/date/weather toggles, and vertical clock positioning. Settings are stored separately in the `desk` NVS namespace, preserving the Stock Clock settings structure. Fresh settings default to Seoul/KST; existing saved timezone and weather location remain in use.

Build uses ESP-IDF 5.5.1, ESP32-C2, 26 MHz crystal, DIO/60 MHz flash and the actual 768 KiB app slots. Only the app-only OTA binary is provided. Do not flash a partition table or merged factory binary. OTA replaces the other application slot; it may contain a previous custom firmware rather than the manufacturer's original firmware. The interface labels it as previous firmware accordingly.

The renderer is tested against complete frames and 16-row SPI bands using sanitizers. Image inspection checks the chip, revision, size, segment checksum, application descriptor and SHA-256. These checks cannot establish that the physical display, Wi-Fi, or weather requests work on the user's device.
