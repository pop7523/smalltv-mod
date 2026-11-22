#include "animation.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
namespace animation {
static uint16_t u16(const uint8_t* b){return b[0]|(uint16_t(b[1])<<8);}
static uint32_t u32(const uint8_t* b){return u16(b)|(uint32_t(u16(b+2))<<16);}
bool header(FILE* f,uint16_t& frames){
    uint8_t b[12];if(fread(b,1,12,f)!=12 || memcmp(b,"S8G1",4) || u16(b+4)!=80 || u16(b+6)!=80 || u16(b+10))return false;
    frames=u16(b+8);return frames>0 && frames<=MaxFrames;
}
static bool decode_impl(FILE* f,Frame* frame,uint16_t& duration){
    uint8_t h[6];if(fread(h,1,6,f)!=6)return false;
    duration=u16(h);uint32_t remaining=u32(h+2);
    if(duration<20 || duration>10000 || !remaining || remaining>Pixels*4)return false;
    if(frame)frame->opaque.fill(0);unsigned at=0;
    auto pixel=[&](const uint8_t* p){
        if(p[2]!=0 && p[2]!=255)return false;
        if(frame){frame->colors[at]=u16(p);if(p[2])frame->opaque[at>>3]|=1u<<(at&7);}at++;return true;
    };
    while(remaining){
        int control=fgetc(f);if(control==EOF)return false;remaining--;
        unsigned count=(control&127)+1;if(count>Pixels-at)return false;
        if(control&128){
            uint8_t p[3];if(remaining<3 || fread(p,1,3,f)!=3)return false;remaining-=3;
            for(unsigned i=0;i<count;i++)if(!pixel(p))return false;
        }else{
            if(remaining<count*3)return false;remaining-=count*3;
            for(unsigned i=0;i<count;i++){uint8_t p[3];if(fread(p,1,3,f)!=3 || !pixel(p))return false;}
        }
    }
    return at==Pixels;
}
bool decode(FILE* f,Frame& frame,uint16_t& duration){return decode_impl(f,&frame,duration);}
bool validate(FILE* f,uint16_t& frames){
    if(fseek(f,0,SEEK_END))return false;long size=ftell(f);if(size<18 || size>long(MaxBytes) || fseek(f,0,SEEK_SET))return false;
    if(!header(f,frames))return false;
    for(unsigned i=0;i<frames;i++){uint16_t duration;if(!decode_impl(f,nullptr,duration))return false;}
    return fgetc(f)==EOF && !ferror(f);
}
void composite(const Frame& frame,int left,int top,int x,int y,uint16_t* row,int count){
    if(y<top || y>=top+80)return;
    int start=std::max(x,left),end=std::min(x+count,left+80);
    for(int px=start;px<end;px++){unsigned at=(y-top)*80+px-left;if(frame.visible(at))row[px-x]=frame.colors[at];}
}
}
