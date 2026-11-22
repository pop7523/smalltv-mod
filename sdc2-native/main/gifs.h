#pragma once
#include "port.h"
#include "esp_http_server.h"
void gifs_init();
std::string gifs_state();
std::vector<smalltv::Rect> gifs_tick(const Settings& s,bool online,uint32_t now);
smalltv::Display& gifs_display();
esp_err_t gifs_upload(httpd_req_t* r);
esp_err_t gifs_delete(httpd_req_t* r);
esp_err_t gifs_download(httpd_req_t* r);
esp_err_t gifs_initialize_storage(httpd_req_t* r);
