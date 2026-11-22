#include "model.h"
#include "glcdfont.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cmath>
struct Assets:smalltv::Assets{
    smalltv::AssetHandle resolve(const std::string&)override{return smalltv::InvalidAsset;}
    bool row(smalltv::AssetHandle,int,int,int,uint16_t*,uint8_t*)override{return false;}
};
struct Screen:smalltv::Display{
    std::vector<uint16_t> pixels=std::vector<uint16_t>(240*240);
    unsigned rows=0;
    uint8_t glyphColumn(uint8_t c,uint8_t col)const override{return font[unsigned(c)*5+col];}
    void row(int x,int y,const uint16_t* p,int n)override{assert(x>=0&&y>=0&&y<240&&x+n<=240&&n>0);std::copy(p,p+n,pixels.begin()+y*240+x);rows++;}
};
int main(){
    Settings s,next;assert(valid_settings(s));
    assert(weather_url(s).find("latitude=37.5665&longitude=126.9780")!=std::string::npos);
    s.lat=-0.0001;s.lon=-180;assert(weather_url(s).find("latitude=-0.0001&longitude=-180.0000")!=std::string::npos);s=Settings{};
    for(const char* bad:{"{\"layout\":99}","{\"bg\":\"#zzzzzz\"}","{\"seconds\":1}","{\"lat\":999}","[]"})assert(!parse_settings(bad,s,next));
    assert(parse_settings("{\"bg\":\"#ffffff\",\"lat\":35.1234,\"seconds\":false}",s,next));
    fprintf(stderr,"config parse: bg=%lx seconds=%d lat=%.12f\n",(unsigned long)next.bg,next.seconds,next.lat);
    assert(next.bg==0xffffff&&!next.seconds&&std::abs(next.lat-35.1234)<0.00001);
    Settings roundtrip;assert(parse_settings(settings_json(next),s,roundtrip));assert(roundtrip.bg==next.bg&&std::abs(roundtrip.lat-next.lat)<0.00001);
    tm time={};time.tm_year=126;time.tm_mon=10;time.tm_mday=22;time.tm_hour=23;time.tm_min=59;time.tm_sec=58;time.tm_wday=0;
    unsigned count=0;
    for(int gif_mode=0;gif_mode<=4;gif_mode++)for(int layout=0;layout<3;layout++)for(int offset=-16;offset<=16;offset+=16)for(int flags=0;flags<32;flags++){
        s=Settings{};s.layout=layout;s.offset=offset;s.seconds=flags&1;s.date=(flags>>1)&1;s.weather=(flags>>2)&1;s.h24=(flags>>3)&1;
        s.gif_mode=gif_mode;
        if(flags&1){s.bg=0xf4f7ff;s.fg=0x080d18;}
        smalltv::Theme theme;std::string error;
        std::string raw=make_theme(s,flags&16,"SmallTV-SDC2-ABCD");
        if(!smalltv::parseTheme(raw,theme,error)){fprintf(stderr,"%s\n",error.c_str());return 1;}
        smalltv::Engine engine;engine.setTheme(std::move(theme),0);engine.setValues({{"wx.temp","-12"},{"wx.hum","45"}});
        Assets assets;Screen screen;assert(smalltv::render(engine,engine.update(0,&time),assets,screen));assert(screen.rows==240);
        for(const auto& state:engine.states()){assert(state.bounds.x>=0&&state.bounds.y>=0&&state.bounds.x+state.bounds.w<=240&&state.bounds.y+state.bounds.h<=240);}
        time.tm_sec=59;assert(smalltv::render(engine,engine.update(1000,&time),assets,screen));time.tm_sec=58;count++;
    }
    printf("PASS: %u actual SmallTV parser/compositor layouts, bounds, updates; invalid settings rejected; settings roundtrip\n",count);
}
