#include "desk.h"
desk_config_t g_desk;
void desk_defaults(desk_config_t *s) {
    *s = (desk_config_t){.version=1, .bg=0x080d18, .fg=0xf4f7ff,
        .accent=0x65dbce, .enabled=1, .layout=0, .seconds=1, .date=1, .weather=1};
}
bool desk_valid(const desk_config_t *s) {
    return s->version==1 && s->bg<=0xffffff && s->fg<=0xffffff && s->accent<=0xffffff
        && s->enabled>=0 && s->enabled<=1 && s->layout>=0 && s->layout<=2
        && s->seconds>=0 && s->seconds<=1 && s->date>=0 && s->date<=1
        && s->weather>=0 && s->weather<=1 && s->offset>=-16 && s->offset<=16;
}
