#include "port.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>
#include <cstdlib>

namespace {
class EmptyAssets:public smalltv::Assets {
 public:
    smalltv::AssetHandle resolve(const std::string&)override{return smalltv::InvalidAsset;}
    bool row(smalltv::AssetHandle,int,int,int,uint16_t*,uint8_t*)override{return false;}
};
void display_task(void*){
    smalltv::Engine engine;EmptyAssets assets;unsigned seen=0,weather_seen=~0u;bool was_online=false,initialized=false;
    for(;;){
        Settings s;unsigned generation,wgen;bool online;std::vector<smalltv::ThemeValue> values;
        xSemaphoreTake(state_mutex,portMAX_DELAY);s=settings;generation=settings_generation;online=connected;wgen=weather_generation;
        if(wgen!=weather_seen)values=weather_values;xSemaphoreGive(state_mutex);
        if(!initialized || seen!=generation || online!=was_online){
            smalltv::Theme theme;std::string error;
            if(smalltv::parseTheme(make_theme(s,!online),theme,error)){
                engine.setTheme(std::move(theme),esp_timer_get_time()/1000);seen=generation;was_online=online;initialized=true;
                setenv("TZ",s.timezone,1);tzset();board_apply(s);weather_seen=~0u;
            }
        }
        if(wgen!=weather_seen){
            xSemaphoreTake(state_mutex,portMAX_DELAY);values=weather_values;xSemaphoreGive(state_mutex);
            engine.setValues(std::move(values));weather_seen=wgen;
        }
        time_t now=time(nullptr);tm local;localtime_r(&now,&local);
        auto dirty=engine.update(esp_timer_get_time()/1000,now>1700000000?&local:nullptr);
        if(initialized && display_ok)smalltv::render(engine,dirty,assets,board_display());
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
void restart_task(void*){vTaskDelay(pdMS_TO_TICKS(1200));esp_restart();}
}
void restart_later(){xTaskCreate(restart_task,"restart",2048,nullptr,3,nullptr);}
extern "C" void app_main(){
    // Never erase NVS on an error: it contains the device's Wi-Fi credentials.
    ESP_ERROR_CHECK(nvs_flash_init());state_mutex=xSemaphoreCreateMutex();if(!state_mutex)esp_restart();
    load_settings();
    nvs_handle_t guard=0;uint8_t boots=0;
    if(nvs_open("smalltv",NVS_READWRITE,&guard)==ESP_OK){
        char previous[33]={};size_t n=sizeof(previous);nvs_get_str(guard,"boot_version",previous,&n);
        if(!strcmp(previous,esp_app_get_description()->version))nvs_get_u8(guard,"boot_count",&boots);
        if(boots>=3){
            const auto* prior=esp_ota_get_next_update_partition(nullptr);
            if(prior && esp_ota_set_boot_partition(prior)==ESP_OK){nvs_close(guard);esp_restart();}
        }
        nvs_set_str(guard,"boot_version",esp_app_get_description()->version);nvs_set_u8(guard,"boot_count",boots+1);nvs_commit(guard);
    }
    display_ok=board_init();network_start();web_start();
    if(display_ok)xTaskCreate(display_task,"smalltv-render",16384,nullptr,4,nullptr);
    vTaskDelay(pdMS_TO_TICKS(60000));
    if(guard){if(display_ok){nvs_set_u8(guard,"boot_count",0);nvs_commit(guard);esp_ota_mark_app_valid_cancel_rollback();}nvs_close(guard);}
    if(!display_ok)esp_restart();
}
