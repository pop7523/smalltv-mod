# SmallTV for SDC2 — USB installation

This compiles the original SmallTV application: src/main.cpp, theme engine,
Wi-Fi setup, settings and web portal. No Stock Clock application files are compiled.
Stock Clock is only the hardware reference: ESP32-C2, 26 MHz crystal, LCD pins,
SPI mode 3, RGB order, active-low backlight and DIO/60 MHz flash.

The original 768 KiB slots cannot contain SmallTV. Initial USB installation
changes the table to two 1.5 MiB app slots and 960 KiB LittleFS. The factory
image includes bootloader, table, app and initial filesystem. This replaces
the existing settings/photos/GIFs. The installer first backs up the entire flash.

## Install

1. Unzip the complete artifact into a writable folder. Keep included files together.
2. Connect a data-capable USB/UART connection and identify the COM port.
   Power-only USB requires a 3.3 V UART adapter wired to the board.
3. Install: `python -m pip install esptool==5.1.0`.
4. Run: `python flash_sdc2_usb.py --port COM5` using your actual port.
   No write occurs if chip detection or full-flash backup fails. Keep the backup
   private: it may contain Wi-Fi passwords. Do not erase flash beforehand.
5. Join `SmallTV-Setup` and open http://192.168.4.1 to set 2.4 GHz Wi-Fi.

Write SmallTV_SDC2_USB.factory.bin at 0x0 over USB/UART only.
Do not upload either image through Stock Clock or the manufacturer's updater.
SmallTV_SDC2_NEW_LAYOUT_OTA.bin is only for later updates after this layout is installed.

Three declarative SmallTV themes are bundled: classic, card and minimal.
Clock, Wi-Fi, brightness, rotation and calibration use SmallTV's own settings.
Weather defaults to Seoul and the clock to KST. Theme colors, positions and
weather URL are in the included editable theme sources. Rebuild these using
SmallTV theme tooling and install through its Theme clocks interface.

Physical display, networking and recovery remain unverified on the actual unit.
If serial auto-reset does not work, identify the board's boot/UART connections;
do not guess pins. ESP32-C2 has no native USB flashing interface.
