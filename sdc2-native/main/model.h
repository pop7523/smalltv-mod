#pragma once
#include "ThemeEngine.h"
#include <string>
struct Settings {
    int version=1,layout=0,brightness=80,offset=0;
    int gif_mode=0,gif_x=80,gif_y=132;
    uint32_t bg=0x080d18,fg=0xf4f7ff,accent=0x65dbce;
    bool seconds=true,date=true,weather=true,h24=true,rotate=false,invert=true;
    double lat=37.5665,lon=126.9780;
    char timezone[64]="KST-9";
};
std::string make_theme(const Settings& s,bool offline,const char* ssid="SmallTV-SDC2");
bool valid_settings(const Settings& s);
bool parse_settings(const std::string& body,const Settings& old,Settings& out);
std::string settings_json(const Settings& s);
std::string weather_url(const Settings& s);
