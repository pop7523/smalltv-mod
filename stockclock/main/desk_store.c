#include "desk.h"
#include "nvs.h"
void desk_load(void) {
    desk_defaults(&g_desk);
    nvs_handle_t h;
    if (nvs_open("desk", NVS_READONLY, &h)!=ESP_OK) return;
    desk_config_t s; size_t n=sizeof(s);
    if (nvs_get_blob(h,"view",&s,&n)==ESP_OK && n==sizeof(s) && desk_valid(&s)) g_desk=s;
    nvs_close(h);
}
int desk_save(const desk_config_t *s) {
    if (!desk_valid(s)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t e=nvs_open("desk",NVS_READWRITE,&h);
    if(e!=ESP_OK) return e;
    e=nvs_set_blob(h,"view",s,sizeof(*s));
    if(e==ESP_OK) e=nvs_commit(h);
    nvs_close(h); return e;
}
