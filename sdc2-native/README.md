# SmallTV SDC2 native OTA port

The unmodified SmallTV `src/features/theme/ThemeEngine.cpp` supplies the actual
declarative theme parser, token expansion, layout, dirty rectangles and row
compositor. ArduinoJson is pinned to 7.4.3. This is a clock/weather-focused port
of that engine to native ESP-IDF 5.5.1, with a native SDC2 display/network adapter
and web controls. No Stock Clock application modules are compiled.

SDC2 hardware initialization uses the GPIO and panel sequence documented by
yanirta/sdc2-stock-clock: ESP32-C2, 26 MHz crystal, MOSI 2, SCLK 1, DC 0, reset 3,
CS tied low, active-low backlight 5, SPI mode 3, RGB and inversion on.

## Installation

Use only `SmallTV_SDC2_NATIVE_OTA.bin`, an application-only OTA image smaller
than the actual stock 786432-byte slot. Upload it through the running Stock
Clock web interface's Firmware upload control. The new application retains
the existing partition table and filesystem. Do not upload the USB factory
image or the USB layout's larger OTA image.

After reboot, open the clock's IP address. Settings include three layouts,
clock/background/accent colors, seconds/date/weather toggles, vertical offset,
brightness, rotation, display inversion, 12/24-hour time, POSIX timezone and
weather coordinates. Defaults are Seoul and KST. Native settings are saved
separately in NVS; the ESP-IDF Wi-Fi credential store is reused.

Recovery Wi-Fi is named `SmallTV-SDC2-XXXX`, at http://192.168.4.1. Its details
appear on screen when disconnected. The portal closes after Wi-Fi connects
and reopens after a connection failure. Three failed boots attempt to select
the other valid application slot; that slot may contain an earlier custom
firmware rather than a manufacturer's original image.

## GIF slots

The web UI accepts up to three 80x80 GIFs. Select one slot, turn GIFs off,
or display all three side by side. Individual horizontal position and shared
vertical position are adjustable. GIF mode reserves room below the big clock
and puts the temperature/humidity on a compact footer line.

GIF decoding and disposal/transparency compositing run in the browser using
gifuct-js 2.1.2 (MIT; notice in web/LICENSE-gifuct-js). The browser converts the
result to a validated, run-length encoded RGB565 animation with binary alpha.
The device saves this converted animation, rather than the original GIF bytes.
The entire web converter is bundled locally and works over the device's HTTP
interface without external CDNs or secure-context browser APIs.

Limits: exactly 80x80, 1..300 frames, original GIF at most 2 MiB, converted slot
at most 384 KiB. Delays are clamped to 20..10000 ms; actual playback speed also
depends on display and flash I/O. Three decoded frames use about 40 KiB RAM.
Playback keeps 24 KiB heap headroom and skips a slot if it cannot allocate its
frame buffer.

Files are stored in the existing spiffs data partition. Mounting never formats
the partition automatically. If it cannot be mounted, the web interface offers
an explicit image-storage initialization with an erase warning; this removes
existing files in that data partition. The app partition table and NVS settings
are not changed. Uploads use a staging file, validate every frame before
publication, retain the previous file on invalid/interrupted uploads, and use
a backup rename to recover interrupted replacement on the next mount.

GIF conversion tests compare real disposal-1/2/3 GIFs against Pillow RGBA
references and exercise the actual native decoder under ASan/UBSan, including
truncation, invalid headers, dimensions, alpha and clipping. The device's flash
filesystem and playback still need physical-device verification.

## Other scope and checks

The native web controls drive declarative text/shape clock and weather layouts
through the original SmallTV engine. Desktop layout tests execute that exact
engine with address/undefined-behavior sanitizers. Theme package uploads,
image/font packages, ticker, radar and other full Arduino application modules
are outside this compact port. The larger original Arduino build needs a
different flash layout and is not suitable for stock web OTA.

Public weather readings use Open-Meteo over HTTP. The live weather fetch,
physical display, reconnection and OTA boot still require verification on the
actual device. Build success and image validation alone do not establish them.

Font provenance: Arduino_GFX v1.6.9 `src/font/glcdfont.h`, classic Adafruit GFX
5x7 font; original comments are preserved. The SDK certificate bundle remains
configured to satisfy ESP-IDF's ROM crypto component, while the app's weather
client uses HTTP.
