// Native settings and declarative-theme adapter; rendering is SmallTV's engine.
#include "model.h"
#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>

bool valid_settings(const Settings& s){
    return s.version==1 && s.gif_mode>=0 && s.gif_mode<=4 && s.gif_x>=0 && s.gif_x<=160 && s.gif_y>=0 && s.gif_y<=160 && s.layout>=0 && s.layout<=2 && s.brightness>=0 && s.brightness<=100 &&
        s.offset>=-16 && s.offset<=16 && s.bg<=0xffffff && s.fg<=0xffffff && s.accent<=0xffffff &&
        std::isfinite(s.lat) && std::isfinite(s.lon) && s.lat>=-90 && s.lat<=90 && s.lon>=-180 && s.lon<=180 &&
        s.timezone[0] && memchr(s.timezone,0,sizeof(s.timezone));
}
std::string weather_url(const Settings& s){
    // Integer formatting also works with IDF's nano printf (no float formats).
    char lat[20],lon[20],url[220];
    long a=std::lround(s.lat*10000),b=std::lround(s.lon*10000);
    snprintf(lat,sizeof(lat),"%s%ld.%04ld",a<0?"-":"",std::labs(a)/10000,std::labs(a)%10000);
    snprintf(lon,sizeof(lon),"%s%ld.%04ld",b<0?"-":"",std::labs(b)/10000,std::labs(b)%10000);
    snprintf(url,sizeof(url),"http://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,relative_humidity_2m",lat,lon);
    return url;
}
static void add_config(JsonDocument& d,const Settings& s){
    d["layout"]=s.layout;d["brightness"]=s.brightness;d["offset"]=s.offset;
    d["gif_mode"]=s.gif_mode;d["gif_x"]=s.gif_x;d["gif_y"]=s.gif_y;
    d["seconds"]=s.seconds;d["date"]=s.date;d["weather"]=s.weather;d["h24"]=s.h24;
    d["rotate"]=s.rotate;d["invert"]=s.invert;d["lat"]=s.lat;d["lon"]=s.lon;d["timezone"]=s.timezone;
    for(auto pair:{std::make_pair("bg",s.bg),std::make_pair("fg",s.fg),std::make_pair("accent",s.accent)}){
        char c[8];snprintf(c,sizeof(c),"#%06lx",static_cast<unsigned long>(pair.second&0xffffff));d[pair.first]=c;
    }
}
std::string settings_json(const Settings& s){JsonDocument d;add_config(d,s);std::string out;serializeJson(d,out);return out;}
bool parse_settings(const std::string& body,const Settings& old,Settings& out){
    JsonDocument d;if(deserializeJson(d,body) || !d.is<JsonObject>())return false;
    Settings s=old;
    auto integer=[&](const char* key,int& dest){if(d[key].isNull())return true;if(!d[key].is<int>())return false;dest=d[key].as<int>();return true;};
    auto boolean=[&](const char* key,bool& dest){if(d[key].isNull())return true;if(!d[key].is<bool>())return false;dest=d[key].as<bool>();return true;};
    if(!integer("gif_mode",s.gif_mode)||!integer("gif_x",s.gif_x)||!integer("gif_y",s.gif_y)||!integer("layout",s.layout)||!integer("brightness",s.brightness)||!integer("offset",s.offset)||
       !boolean("seconds",s.seconds)||!boolean("date",s.date)||!boolean("weather",s.weather)||
       !boolean("h24",s.h24)||!boolean("rotate",s.rotate)||!boolean("invert",s.invert))return false;
    const char* keys[]={"bg","fg","accent"};uint32_t* destinations[]={&s.bg,&s.fg,&s.accent};
    for(int index=0;index<3;index++){
        if(d[keys[index]].isNull())continue;
        if(!d[keys[index]].is<const char*>())return false;
        const char* c=d[keys[index]];if(strlen(c)!=7 || c[0]!='#')return false;
        for(int i=1;i<7;i++)if(!isxdigit(static_cast<unsigned char>(c[i])))return false;
        *destinations[index]=strtoul(c+1,nullptr,16);
    }
    if(!d["lat"].isNull()){if(!d["lat"].is<double>())return false;s.lat=d["lat"].as<double>();}
    if(!d["lon"].isNull()){if(!d["lon"].is<double>())return false;s.lon=d["lon"].as<double>();}
    if(!d["timezone"].isNull()){
        if(!d["timezone"].is<const char*>())return false;
        const char* tz=d["timezone"];if(!tz[0]||strlen(tz)>=sizeof(s.timezone))return false;
        strcpy(s.timezone,tz);
    }
    if(!valid_settings(s))return false;
    out=s;return true;
}
std::string make_theme(const Settings& s,bool offline,const char* ssid){
    JsonDocument d;d["spec"]=2;auto meta=d["theme"].to<JsonObject>();meta["id"]="sdc2-native";meta["name"]="SmallTV SDC2";meta["author"]="SmallTV SDC2 port";meta["version"]="1.0.0";
    auto screen=d["display"].to<JsonObject>();screen["width"]=240;screen["height"]=240;
    char bg[8],fg[8],acc[8];snprintf(bg,8,"#%06lx",(unsigned long)(s.bg&0xffffff));snprintf(fg,8,"#%06lx",(unsigned long)(s.fg&0xffffff));snprintf(acc,8,"#%06lx",(unsigned long)(s.accent&0xffffff));screen["background"]=bg;
    auto data=d["data"].to<JsonArray>();auto wx=data.add<JsonObject>();wx["id"]="wx";wx["url"]="http://api.open-meteo.com";wx["interval"]=600;
    auto fields=wx["fields"].to<JsonArray>();for(auto pair:{std::make_pair("temp","current.temperature_2m"),std::make_pair("hum","current.relative_humidity_2m")}){auto f=fields.add<JsonObject>();f["id"]=pair.first;f["path"]=pair.second;f["decimals"]=0;}
    auto layers=d["layers"].to<JsonArray>();
    auto text=[&](const char* id,const char* value,int y,int size,const char* color){auto l=layers.add<JsonObject>();l["id"]=id;l["type"]="text";l["x"]=120;l["y"]=y;l["anchor"]="center";l["value"]=value;l["size"]=size;l["color"]=color;};
    if(s.date)text("date","{WD} {DD} {MON}",18,16,fg);
    int clock_y=(s.layout==1?75:s.layout==2?110:92)+s.offset;
    if(s.gif_mode)clock_y=75+s.offset;
    text("time",s.h24?"{HH}:{MM}":"{hh}:{MM}",clock_y,56,fg);
    if(s.seconds){text("seconds","{SS}",18,16,acc);layers[layers.size()-1]["x"]=220;}
    if(offline){text("wifi",ssid,187,12,acc);text("address","192.168.4.1",218,16,fg);}
    else if(s.weather && s.gif_mode){text("weather","{wx.temp} C | {wx.hum}%",226,12,fg);}
    else if(s.weather){
        if(s.layout==1){auto card=layers.add<JsonObject>();card["id"]="card";card["type"]="shape";card["shape"]="rectangle";card["x"]=8;card["y"]=155;card["width"]=224;card["height"]=77;card["cornerRadius"]=10;card["fill"]="#152c33";}
        text("temperature","{wx.temp} C",184,24,acc);text("humidity","Humidity {wx.hum}%",218,12,fg);
    }
    std::string json;serializeJson(d,json);return json;
}
