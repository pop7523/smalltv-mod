#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
namespace animation {
constexpr unsigned Width=80,Height=80,Pixels=6400,MaxFrames=300,MaxBytes=384*1024;
struct Frame {
    std::array<uint16_t,Pixels> colors{};
    std::array<uint8_t,Pixels/8> opaque{};
    bool visible(unsigned p)const{return opaque[p>>3]&(1u<<(p&7));}
};
bool header(FILE* f,uint16_t& frames);
bool decode(FILE* f,Frame& frame,uint16_t& duration);
bool validate(FILE* f,uint16_t& frames);
void composite(const Frame& frame,int left,int top,int x,int y,uint16_t* row,int count);
}
