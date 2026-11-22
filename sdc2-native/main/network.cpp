// Native network adapter for the portable SmallTV theme engine.
#include "port.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include <cstdio>
#include <cstring>

namespace {
bool sntp_started=false;
void event(void*,esp_event_base_t base,int32_t id,void* data){
    if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP){
        auto* e=static_cast<ip_event_got_ip_t*>(data);
        xSemaphoreTake(state_mutex,portMAX_DELAY);connected=true;
        snprintf(device_ip,sizeof(device_ip),IPSTR,IP2STR(&e->ip_info.ip));xSemaphoreGive(state_mutex);
        if(!sntp_started){auto config=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");if(esp_netif_sntp_init(&config)==ESP_OK)sntp_started=true;}
    }else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED){
        xSemaphoreTake(state_mutex,portMAX_DELAY);connected=false;device_ip[0]=0;xSemaphoreGive(state_mutex);
    }
}
void worker(void*){
    int64_t retry=0,next_weather=0,online_since=0,offline_since=0;double lat=1000,lon=1000;bool ap_enabled=true;
    for(;;){
        Settings s;bool online;
        xSemaphoreTake(state_mutex,portMAX_DELAY);s=settings;online=connected;xSemaphoreGive(state_mutex);
        int64_t now=esp_timer_get_time();
        if(online){
            offline_since=now;
            if(!online_since)online_since=now;
            if(ap_enabled && now-online_since>20000000 && esp_wifi_set_mode(WIFI_MODE_STA)==ESP_OK)ap_enabled=false;
        }else{
            online_since=0;
            if(!offline_since)offline_since=now;
            if(!ap_enabled && now-offline_since>25000000 && esp_wifi_set_mode(WIFI_MODE_APSTA)==ESP_OK)ap_enabled=true;
        }
        if(!online && now>=retry){esp_wifi_connect();retry=now+30000000;}
        if(s.lat!=lat || s.lon!=lon){next_weather=0;lat=s.lat;lon=s.lon;}
        if(online && s.weather && now>=next_weather){
            char url[220];snprintf(url,sizeof(url),"http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m",s.lat,s.lon);
            esp_http_client_config_t config={};config.url=url;config.timeout_ms=8000;config.disable_auto_redirect=true;
            auto client=esp_http_client_init(&config);bool ok=false;std::vector<smalltv::ThemeValue> result;
            if(client){
                if(esp_http_client_open(client,0)==ESP_OK){
                    int64_t length=esp_http_client_fetch_headers(client);
                    if(esp_http_client_get_status_code(client)==200 && length<=4096){
                        std::string response;char buffer[256];int64_t deadline=esp_timer_get_time()+12000000;
                        while(response.size()<4096 && esp_timer_get_time()<deadline){
                            int n=esp_http_client_read(client,buffer,sizeof(buffer));if(n<=0)break;response.append(buffer,n);
                        }
                        smalltv::ThemeDataSource source;source.id="wx";source.listIndexes=true;
                        source.fields.push_back({"temp","current.temperature_2m",0});source.fields.push_back({"hum","current.relative_humidity_2m",0});
                        ok=smalltv::extractThemeValues(response,source,result,8192);
                    }
                    esp_http_client_close(client);
                }esp_http_client_cleanup(client);
            }
            xSemaphoreTake(state_mutex,portMAX_DELAY);
            if(s.lat==settings.lat && s.lon==settings.lon){
                if(ok){weather_values=std::move(result);weather_generation++;weather_error[0]=0;}
                else strcpy(weather_error,"Weather request failed; retrying");
            }xSemaphoreGive(state_mutex);
            next_weather=esp_timer_get_time()+(ok?600000000LL:60000000LL);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
}
esp_err_t network_credentials(const char* ssid,const char* pass){
    wifi_config_t c={};strlcpy(reinterpret_cast<char*>(c.sta.ssid),ssid,sizeof(c.sta.ssid));
    strlcpy(reinterpret_cast<char*>(c.sta.password),pass,sizeof(c.sta.password));
    c.sta.scan_method=WIFI_ALL_CHANNEL_SCAN;c.sta.threshold.authmode=WIFI_AUTH_OPEN;
    esp_wifi_disconnect();esp_err_t e=esp_wifi_set_config(WIFI_IF_STA,&c);if(e==ESP_OK)esp_wifi_connect();return e;
}
void network_start(){
    ESP_ERROR_CHECK(esp_netif_init());ESP_ERROR_CHECK(esp_event_loop_create_default());
    auto station=esp_netif_create_default_wifi_sta();esp_netif_create_default_wifi_ap();
    esp_netif_set_hostname(station,"smalltv-sdc2");
    auto config=WIFI_INIT_CONFIG_DEFAULT();ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event,nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event,nullptr));
    uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_SOFTAP);snprintf(setup_ssid,sizeof(setup_ssid),"SmallTV-SDC2-%02X%02X",mac[4],mac[5]);
    wifi_config_t ap={};strcpy(reinterpret_cast<char*>(ap.ap.ssid),setup_ssid);ap.ap.ssid_len=strlen(setup_ssid);ap.ap.channel=6;ap.ap.max_connection=3;ap.ap.authmode=WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,&ap));
    ESP_ERROR_CHECK(esp_wifi_start());esp_wifi_set_ps(WIFI_PS_NONE);
    xTaskCreate(worker,"smalltv-network",8192,nullptr,3,nullptr);
}
