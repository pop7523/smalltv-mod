#include "animation.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>
static FILE* file(const std::vector<uint8_t>& bytes){FILE* f=tmpfile();assert(f);assert(fwrite(bytes.data(),1,bytes.size(),f)==bytes.size());rewind(f);return f;}
int main(int argc,char** argv){
    assert(argc==3);FILE* input=fopen(argv[1],"rb");assert(input);uint16_t frames=0;assert(animation::validate(input,frames));rewind(input);assert(animation::header(input,frames));
    FILE* reference=fopen(argv[2],"rb");assert(reference);auto frame=std::make_unique<animation::Frame>();
    for(int n=0;n<frames;n++){
        uint16_t duration;assert(animation::decode(input,*frame,duration));
        for(unsigned p=0;p<6400;p++){
            uint8_t rgba[4];assert(fread(rgba,1,4,reference)==4);bool visible=rgba[3]>=128;assert(frame->visible(p)==visible);
            uint16_t expected=((rgba[0]&248)<<8)|((rgba[1]&252)<<3)|(rgba[2]>>3);if(visible)assert(frame->colors[p]==expected);
        }
    }assert(fgetc(input)==EOF&&fgetc(reference)==EOF);fclose(reference);
    fseek(input,0,SEEK_END);long length=ftell(input);rewind(input);std::vector<uint8_t> bytes(length);assert(fread(bytes.data(),1,bytes.size(),input)==bytes.size());fclose(input);
    for(size_t end:{size_t(0),size_t(11),size_t(12),bytes.size()-1}){std::vector<uint8_t> bad(bytes.begin(),bytes.begin()+end);FILE* f=file(bad);assert(!animation::validate(f,frames));fclose(f);}
    for(int pos:{0,4,6,8,10}){auto bad=bytes;bad[pos]=0xff;FILE* f=file(bad);assert(!animation::validate(f,frames));fclose(f);}
    auto bad=bytes;bad.push_back(0);FILE* f=file(bad);assert(!animation::validate(f,frames));fclose(f);
    frame->opaque.fill(255);frame->colors.fill(0xf800);uint16_t row[240];for(auto& p:row)p=0x07e0;
    animation::composite(*frame,80,120,0,130,row,240);
    for(int i=0;i<240;i++)assert(row[i]==(i>=80&&i<160?0xf800:0x07e0));
    frame->opaque.fill(0);animation::composite(*frame,0,0,0,0,row,240);assert(row[0]==0x07e0);
    printf("PASS: GIF conversion parity, %u frames; alpha/clipping, truncated and invalid file rejection\n",frames);
}
