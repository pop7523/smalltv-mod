#include "port.h"
#include "nvs.h"
Settings settings;
SemaphoreHandle_t state_mutex;
unsigned settings_generation=1,weather_generation=0;
bool connected=false,display_ok=false;
char device_ip[16]="",setup_ssid[33]="",weather_error[64]="Waiting for weather";
std::vector<smalltv::ThemeValue> weather_values;

esp_err_t save_settings(const Settings& s){
    if(!valid_settings(s))return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;esp_err_t err=nvs_open("smalltv",NVS_READWRITE,&h);if(err!=ESP_OK)return err;
    std::string json=settings_json(s);err=nvs_set_str(h,"native_cfg",json.c_str());
    if(err==ESP_OK)err=nvs_commit(h);nvs_close(h);return err;
}
void load_settings(){
    nvs_handle_t h;if(nvs_open("smalltv",NVS_READONLY,&h)!=ESP_OK)return;
    size_t n=0;if(nvs_get_str(h,"native_cfg",nullptr,&n)==ESP_OK && n>1 && n<=2048){
        std::string raw(n,'\0');if(nvs_get_str(h,"native_cfg",&raw[0],&n)==ESP_OK){raw.resize(n-1);Settings parsed;if(parse_settings(raw,settings,parsed))settings=parsed;}
    }nvs_close(h);
}
