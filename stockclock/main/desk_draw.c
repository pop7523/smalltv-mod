#include "desk.h"
static uint16_t color(uint32_t v) { return rgb(v>>16,v>>8,v); }
static void text(canvas_t *c,const font_t *font,int y,const char *v,uint16_t fg) {
    while(gfx_text_width(font,v)>224 && font!=&font_s) {
        font=font==&font_xxl?&font_xl:font==&font_xl?&font_l:font==&font_l?&font_m:&font_s;
    }
    gfx_text(c,font,120,y,v,fg,ALIGN_CENTER);
}
void desk_draw(canvas_t *c,const desk_config_t *s,const desk_view_t *v) {
    uint16_t bg=color(s->bg),fg=color(s->fg),acc=color(s->accent),dim=gfx_mix(bg,fg,160);
    gfx_fill(c,bg);
    int y=(s->layout==1?28:s->layout==2?70:58)+s->offset;
    if(s->date) text(c,&font_s,s->layout==1?3:28,v->date,dim);
    text(c,&font_xxl,y,v->clock,fg);
    if(s->seconds) gfx_text(c,&font_s,228,4,v->seconds,acc,ALIGN_RIGHT);
    if(!s->weather) return;
    if(s->layout==1) gfx_round_rect(c,8,141,224,91,12,gfx_mix(bg,acc,26));
    if(s->layout!=2) {
        gfx_hline(c,72,151,96,acc);
        text(c,&font_l,163,v->weather_ready?v->temp:"--",acc);
        text(c,&font_s,205,v->weather_ready?v->condition:"Weather loading",dim);
    } else {
        text(c,&font_m,185,v->weather_ready?v->temp:"--",acc);
        text(c,&font_s,216,v->weather_ready?v->condition:"Weather loading",dim);
    }
}
