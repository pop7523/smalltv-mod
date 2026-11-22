#pragma once
#include "ThemeEngine.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string>

struct Settings {
    int version=1,layout=0,brightness=80,offset=0;
    uint32_t bg=0x080d18,fg=0xf4f7ff,accent=0x65dbce;
    bool seconds=true,date=true,weather=true,h24=true,rotate=false,invert=true;
    double lat=37.5665,lon=126.9780;
    char timezone[64]="KST-9";
};
extern Settings settings;
extern SemaphoreHandle_t state_mutex;
extern unsigned settings_generation,weather_generation;
extern bool connected,display_ok;
extern char device_ip[16],setup_ssid[33],weather_error[64];
extern std::vector<smalltv::ThemeValue> weather_values;
std::string make_theme(const Settings& s,bool offline);
bool valid_settings(const Settings& s);
esp_err_t save_settings(const Settings& s);
void load_settings();
bool parse_settings(const std::string& body,const Settings& old,Settings& out);
std::string settings_json(const Settings& s);
bool board_init();
void board_apply(const Settings& s);
smalltv::Display& board_display();
void network_start();
esp_err_t network_credentials(const char* ssid,const char* pass);
void web_start();
void restart_later();
