#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "desk.h"
static uint16_t full[240*240], strips[240*240];
int main(void) {
    desk_config_t s; desk_defaults(&s); assert(desk_valid(&s));
    s.layout=3; assert(!desk_valid(&s)); desk_defaults(&s);
    s.offset=-17; assert(!desk_valid(&s)); desk_defaults(&s);
    desk_view_t v={.clock="23:59",.seconds="58",.date="Sun 22 Nov",.temp="-12" GLYPH_DEG "C",.condition="Thunderstorm",.weather_ready=true};
    unsigned count=0;
    for(int layout=0;layout<3;layout++) for(int offset=-16;offset<=16;offset+=16)
    for(int flags=0;flags<16;flags++) {
        s.layout=layout;s.offset=offset;s.date=flags&1;s.seconds=(flags>>1)&1;s.weather=(flags>>2)&1;
        v.weather_ready=(flags>>3)&1;
        if(flags&1) {s.bg=0xf4f7ff;s.fg=0x080d18;} else {s.bg=0x080d18;s.fg=0xf4f7ff;}
        assert(desk_valid(&s));
        canvas_t c={full,0,240};desk_draw(&c,&s,&v);
        for(int y=0;y<240;y+=16) {canvas_t band={strips+y*240,y,16};desk_draw(&band,&s,&v);}
        assert(!memcmp(full,strips,sizeof(full)));count++;
    }
    printf("PASS: %u full-frame versus strip render comparisons; layout bounds validated\n",count);
    return 0;
}
