#include "port.h"
#include "gifs.h"
#include <ArduinoJson.h>
#include "esp_http_server.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <cstring>

extern const char html_start[] asm("_binary_index_html_start");
extern const char html_end[] asm("_binary_index_html_end");
extern const char gif_js_start[] asm("_binary_gif_codec_js_start");
extern const char gif_js_end[] asm("_binary_gif_codec_js_end");
namespace {
esp_err_t reply(httpd_req_t* r,const std::string& s){httpd_resp_set_type(r,"application/json");httpd_resp_set_hdr(r,"Cache-Control","no-store");return httpd_resp_send(r,s.data(),s.size());}
bool read(httpd_req_t* r,std::string& body){
    if(r->content_len<1 || r->content_len>2048)return false;body.resize(r->content_len);size_t got=0;
    while(got<body.size()){int n=httpd_req_recv(r,&body[got],body.size()-got);if(n<=0)return false;got+=n;}return true;
}
esp_err_t index(httpd_req_t* r){httpd_resp_set_type(r,"text/html; charset=utf-8");httpd_resp_set_hdr(r,"Cache-Control","no-store");return httpd_resp_send(r,html_start,html_end-html_start);}
esp_err_t gif_js(httpd_req_t* r){httpd_resp_set_type(r,"text/javascript; charset=utf-8");return httpd_resp_send(r,gif_js_start,gif_js_end-gif_js_start);}
esp_err_t gif_state(httpd_req_t* r){return reply(r,gifs_state());}
esp_err_t state(httpd_req_t* r){
    JsonDocument d;Settings s;
    xSemaphoreTake(state_mutex,portMAX_DELAY);s=settings;d["connected"]=connected;d["ip"]=device_ip;d["ap"]=setup_ssid;d["display_ok"]=display_ok;d["weather_error"]=weather_error;
    for(const auto& v:weather_values)d["values"][v.key]=v.value;
    xSemaphoreGive(state_mutex);
    JsonDocument cfg;deserializeJson(cfg,settings_json(s));d["cfg"]=cfg.as<JsonVariant>();
    d["version"]=esp_app_get_description()->version;d["engine"]="smalltv-mod ThemeEngine.cpp";d["heap"]=esp_get_free_heap_size();
    d["slot_bytes"]=esp_ota_get_running_partition()->size;
    std::string out;serializeJson(d,out);return reply(r,out);
}
esp_err_t configure(httpd_req_t* r){
    std::string body;if(!read(r,body))return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Invalid body");
    Settings old,next;xSemaphoreTake(state_mutex,portMAX_DELAY);old=settings;xSemaphoreGive(state_mutex);
    if(!parse_settings(body,old,next))return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Invalid setting");
    smalltv::Theme theme;std::string error;if(!smalltv::parseTheme(make_theme(next,false),theme,error))return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,error.c_str());
    if(save_settings(next)!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Settings could not be saved");
    xSemaphoreTake(state_mutex,portMAX_DELAY);settings=next;settings_generation++;xSemaphoreGive(state_mutex);
    return reply(r,"{\"ok\":true}");
}
esp_err_t wifi(httpd_req_t* r){
    std::string body;JsonDocument d;if(!read(r,body)||deserializeJson(d,body)||!d["ssid"].is<const char*>()||!d["pass"].is<const char*>())return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"ssid and pass required");
    const char* ssid=d["ssid"],*pass=d["pass"];if(!ssid[0]||strlen(ssid)>32||strlen(pass)>64)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Invalid Wi-Fi credentials");
    if(network_credentials(ssid,pass)!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Wi-Fi save failed");
    return reply(r,"{\"ok\":true}");
}
esp_err_t ota(httpd_req_t* r){
    const esp_partition_t* slot=esp_ota_get_next_update_partition(nullptr);
    if(!slot || r->content_len<288 || unsigned(r->content_len)>slot->size)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Not an app image or too large for OTA slot");
    unsigned char header[24];int got=0;
    while(got<24){int n=httpd_req_recv(r,reinterpret_cast<char*>(header)+got,24-got);if(n<=0)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Incomplete image header");got+=n;}
    if(header[0]!=0xe9 || header[12]!=12 || header[13]!=0)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"ESP32-C2 app-only image required");
    esp_ota_handle_t handle;esp_err_t error=esp_ota_begin(slot,r->content_len,&handle);
    if(error!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"OTA begin failed");
    error=esp_ota_write(handle,header,sizeof(header));int remaining=r->content_len-24;char buffer[1024];
    int64_t deadline=esp_timer_get_time()+180000000;
    while(error==ESP_OK && remaining>0 && esp_timer_get_time()<deadline){
        int n=httpd_req_recv(r,buffer,remaining>int(sizeof(buffer))?sizeof(buffer):remaining);
        if(n<=0){error=ESP_FAIL;break;}error=esp_ota_write(handle,buffer,n);remaining-=n;
    }
    if(error!=ESP_OK || remaining){esp_ota_abort(handle);return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"OTA interrupted");}
    error=esp_ota_end(handle);if(error==ESP_OK)error=esp_ota_set_boot_partition(slot);
    if(error!=ESP_OK)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Image validation failed");
    reply(r,"{\"ok\":true}");restart_later();return ESP_OK;
}
esp_err_t reboot(httpd_req_t* r){reply(r,"{\"ok\":true}");restart_later();return ESP_OK;}
}
void web_start(){
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.stack_size=8192;config.max_uri_handlers=14;config.lru_purge_enable=true;config.recv_wait_timeout=15;
    httpd_handle_t server;ESP_ERROR_CHECK(httpd_start(&server,&config));
    auto route=[&](const char* url,httpd_method_t method,esp_err_t(*handler)(httpd_req_t*)){httpd_uri_t u={};u.uri=url;u.method=method;u.handler=handler;ESP_ERROR_CHECK(httpd_register_uri_handler(server,&u));};
    route("/",HTTP_GET,index);route("/api/state",HTTP_GET,state);route("/api/settings",HTTP_POST,configure);
    route("/api/wifi",HTTP_POST,wifi);route("/ota",HTTP_POST,ota);route("/api/reboot",HTTP_POST,reboot);
    route("/gif-codec.js",HTTP_GET,gif_js);route("/api/gifs",HTTP_GET,gif_state);
    route("/api/gif",HTTP_POST,gifs_upload);route("/api/gif",HTTP_DELETE,gifs_delete);route("/api/gif",HTTP_GET,gifs_download);
    route("/api/gifs/initialize",HTTP_POST,gifs_initialize_storage);
}
