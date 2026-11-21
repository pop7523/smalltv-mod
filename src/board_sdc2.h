// Experimental JUZIPi SDC2 wiring, from yanirta/sdc2-stock-clock README.
// Not the GeekMagic C2 board. No hardware validation has been performed.
#pragma once
#if !defined(SMALLTV_ESP32C2)
#error "SDC2 requires the ESP32-C2 target"
#endif
#include <sdkconfig.h>
#if !defined(CONFIG_XTAL_FREQ_26) || !CONFIG_XTAL_FREQ_26
#error "SDC2 requires IDF libraries built for a 26 MHz crystal"
#endif
#define TFT_SCLK 1
#define TFT_MOSI 2
#define TFT_DC 0
#define TFT_RST 3
#define TFT_CS -1
#define TFT_BL 5
#define TFT_BGR 0
#define TFT_BL_DEFAULT_INVERTED true
#define HAS_LDR 0
#define ADC_MAX 4095
