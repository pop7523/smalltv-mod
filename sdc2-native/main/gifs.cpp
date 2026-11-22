#include "gifs.h"
#include "animation.h"
#include <ArduinoJson.h>
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
#include <sys/stat.h>
#include <unistd.h>
namespace {
SemaphoreHandle_t storage_mutex;
bool ready=false;
unsigned generation=1;
constexpr const char* stage="/gifs/.upload";
std::string path(int slot){return "/gifs/smalltv-"+std::to_string(slot)+".s8g";}
class Lock{public:Lock(){xSemaphoreTake(storage_mutex,portMAX_DELAY);}~Lock(){xSemaphoreGive(storage_mutex);}};
bool mount(bool initialize){
    esp_vfs_spiffs_conf_t c={};c.base_path="/gifs";c.partition_label="spiffs";c.max_files=5;c.format_if_mount_failed=initialize;
    if(esp_vfs_spiffs_register(&c)!=ESP_OK)return false;
    unlink(stage);
    for(int i=1;i<=3;i++){
        auto current=path(i),backup=current+".bak";struct stat st;
        if(stat(current.c_str(),&st) && !stat(backup.c_str(),&st))rename(backup.c_str(),current.c_str());
        else unlink(backup.c_str());
    }return true;
}
int slot_for(httpd_req_t* r){char query[32],value[4];if(httpd_req_get_url_query_str(r,query,sizeof(query))!=ESP_OK || httpd_query_key_value(query,"slot",value,sizeof(value))!=ESP_OK)return 0;return strlen(value)==1 && value[0]>='1' && value[0]<='3'?value[0]-'0':0;}
esp_err_t fail(httpd_req_t* r,const char* text){return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,text);}
struct Player{
    std::unique_ptr<animation::Frame> frame;
    uint32_t next=0;long cursor=12;uint16_t count=0,index=0;bool valid=false;int x=0,y=132,slot=0;
};
Player players[3];unsigned seen=0;int last_mode=-1,last_x=-1,last_y=-1;
class Overlay:public smalltv::Display{
 public:
    uint8_t glyphColumn(uint8_t c,uint8_t col)const override{return board_display().glyphColumn(c,col);}
    void row(int x,int y,const uint16_t* input,int count)override{
        uint16_t colors[240];if(count<1 || count>240)return;
        std::copy(input,input+count,colors);
        for(const auto& p:players)if(p.valid && p.frame)animation::composite(*p.frame,p.x,p.y,x,y,colors,count);
        board_display().row(x,y,colors,count);
    }
} overlay;
}
void gifs_init(){storage_mutex=xSemaphoreCreateMutex();if(storage_mutex)ready=mount(false);}
smalltv::Display& gifs_display(){return overlay;}
std::string gifs_state(){
    JsonDocument d;if(!storage_mutex){d["ready"]=false;std::string s;serializeJson(d,s);return s;}
    Lock lock;d["ready"]=ready;d["max_bytes"]=animation::MaxBytes;d["max_frames"]=animation::MaxFrames;
    auto slots=d["slots"].to<JsonArray>();
    for(int i=1;i<=3;i++){
        auto entry=slots.add<JsonObject>();entry["slot"]=i;entry["bytes"]=0;entry["frames"]=0;
        if(!ready)continue;struct stat st;if(stat(path(i).c_str(),&st))continue;
        FILE* f=fopen(path(i).c_str(),"rb");uint16_t frames=0;
        if(f){if(animation::header(f,frames)){entry["bytes"]=st.st_size;entry["frames"]=frames;}fclose(f);}
    }
    std::string s;serializeJson(d,s);return s;
}
std::vector<smalltv::Rect> gifs_tick(const Settings& s,bool online,uint32_t now){
    std::vector<smalltv::Rect> dirty;if(!storage_mutex)return dirty;
    Lock lock;int mode=online?s.gif_mode:0;
    if(generation!=seen || mode!=last_mode || s.gif_x!=last_x || s.gif_y!=last_y){
        for(auto& p:players){if(p.valid)dirty.emplace_back(p.x,p.y,80,80);p=Player{};}
        if(ready && mode){
            for(int i=0;i<3;i++){
                if(mode!=4 && mode!=i+1)continue;
                auto& p=players[i];FILE* f=fopen(path(i+1).c_str(),"rb");if(!f)continue;
                bool ok=animation::header(f,p.count);fclose(f);if(!ok)continue;
                if(heap_caps_get_free_size(MALLOC_CAP_8BIT)<sizeof(animation::Frame)+24000)continue;
                p.frame.reset(new(std::nothrow) animation::Frame);if(!p.frame)continue;
                p.slot=i+1;p.x=mode==4?i*80:s.gif_x;p.y=s.gif_y;
            }
        }
        seen=generation;last_mode=mode;last_x=s.gif_x;last_y=s.gif_y;
    }
    for(auto& p:players){
        if(!p.frame || !p.slot || (p.valid && int32_t(now-p.next)<0))continue;
        FILE* f=fopen(path(p.slot).c_str(),"rb");bool ok=false;uint16_t duration=100;
        if(f){if(!fseek(f,p.cursor,SEEK_SET))ok=animation::decode(f,*p.frame,duration);p.cursor=ftell(f);fclose(f);}
        dirty.emplace_back(p.x,p.y,80,80);
        if(!ok){p.valid=false;p.slot=0;continue;}
        p.valid=true;p.next=now+duration;if(++p.index>=p.count){p.index=0;p.cursor=12;}
    }
    return dirty;
}
esp_err_t gifs_initialize_storage(httpd_req_t* r){
    if(!storage_mutex)return fail(r,"Storage unavailable");Lock lock;
    if(ready)return fail(r,"Storage is already mounted; existing files were kept");
    // This path is only called after the user confirms the browser's erase warning.
    if(!r->content_len || r->content_len>16)return fail(r,"Initialization confirmation required");
    char body[17]={};int n=httpd_req_recv(r,body,r->content_len);
    if(n!=r->content_len || strcmp(body,"INITIALIZE"))return fail(r,"Initialization confirmation required");
    ready=mount(true);generation++;
    if(!ready)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Could not initialize image storage");
    return httpd_resp_sendstr(r,"{\"ok\":true}");
}
esp_err_t gifs_upload(httpd_req_t* r){
    int slot=slot_for(r);if(!slot || !storage_mutex || !ready || r->content_len<18 || unsigned(r->content_len)>animation::MaxBytes)return fail(r,"80x80 animation, slot 1..3, max 384 KiB required");
    FILE* f=nullptr;{
        Lock lock;size_t total=0,used=0;
        if(esp_spiffs_info("spiffs",&total,&used)!=ESP_OK || total-used<unsigned(r->content_len)+16384)return fail(r,"Not enough image storage; delete another GIF first");
        f=fopen(stage,"wb");
    }
    if(!f)return fail(r,"Could not create upload file");
    char buffer[1024];int remaining=r->content_len;bool ok=true;
    while(remaining>0){
        int n=httpd_req_recv(r,buffer,std::min(remaining,int(sizeof(buffer))));if(n<=0){ok=false;break;}
        {Lock lock;if(fwrite(buffer,1,n,f)!=unsigned(n))ok=false;}
        if(!ok)break;remaining-=n;
    }
    {Lock lock;if(fclose(f))ok=false;}
    uint16_t frames=0;
    if(ok){Lock lock;f=fopen(stage,"rb");ok=f && animation::validate(f,frames);if(f)fclose(f);}
    {
        Lock lock;
        if(ok){
            auto current=path(slot),backup=current+".bak";unlink(backup.c_str());
            struct stat st;bool had=!stat(current.c_str(),&st);
            if(had && rename(current.c_str(),backup.c_str()))ok=false;
            if(ok && rename(stage,current.c_str())){ok=false;if(had)rename(backup.c_str(),current.c_str());}
            if(ok){unlink(backup.c_str());generation++;}
        }
        if(!ok)unlink(stage);
    }
    if(!ok)return fail(r,"Invalid/truncated animation or write failed; previous GIF retained");
    return httpd_resp_sendstr(r,"{\"ok\":true}");
}
esp_err_t gifs_delete(httpd_req_t* r){
    int slot=slot_for(r);if(!slot || !storage_mutex || !ready)return fail(r,"Invalid slot or storage unavailable");
    Lock lock;unlink(path(slot).c_str());generation++;return httpd_resp_sendstr(r,"{\"ok\":true}");
}
esp_err_t gifs_download(httpd_req_t* r){
    int slot=slot_for(r);if(!slot || !ready)return fail(r,"Invalid slot or storage unavailable");
    FILE* f;{Lock lock;f=fopen(path(slot).c_str(),"rb");}if(!f)return httpd_resp_send_err(r,HTTPD_404_NOT_FOUND,"Empty slot");
    httpd_resp_set_type(r,"application/octet-stream");httpd_resp_set_hdr(r,"Cache-Control","no-store");char b[1024];size_t n;esp_err_t error=ESP_OK;
    while((n=fread(b,1,sizeof(b),f))){error=httpd_resp_send_chunk(r,b,n);if(error!=ESP_OK)break;}
    fclose(f);if(error==ESP_OK)error=httpd_resp_send_chunk(r,nullptr,0);return error;
}
