#pragma once
#include "gfx.h"
typedef struct {
    uint32_t version, bg, fg, accent;
    int enabled, layout, seconds, date, weather, offset;
} desk_config_t;
typedef struct {
    char clock[24], seconds[4], date[32], temp[16], condition[32], place[32];
    bool weather_ready;
} desk_view_t;
extern desk_config_t g_desk;
void desk_defaults(desk_config_t *s);
bool desk_valid(const desk_config_t *s);
void desk_load(void);
int desk_save(const desk_config_t *s);
void desk_draw(canvas_t *c, const desk_config_t *s, const desk_view_t *v);
