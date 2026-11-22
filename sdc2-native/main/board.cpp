// SDC2 hardware adapter for SmallTV's original row compositor.
// GPIO/clock/panel initialization reference: yanirta/sdc2-stock-clock.
#include "port.h"
#include "glcdfont.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_heap_caps.h"
#include <cstring>

namespace {
esp_lcd_panel_handle_t panel;
SemaphoreHandle_t done;
uint16_t* dma;
bool transferred(esp_lcd_panel_io_handle_t,esp_lcd_panel_io_event_data_t*,void*) {
    BaseType_t wake=pdFALSE;xSemaphoreGiveFromISR(done,&wake);return wake==pdTRUE;
}
class Panel:public smalltv::Display {
 public:
    uint8_t glyphColumn(uint8_t c,uint8_t col)const override{return font[unsigned(c)*5+col];}
    void row(int x,int y,const uint16_t* colors,int count)override {
        if(!display_ok || count<1 || count>240 || x<0 || x+count>240 || y<0 || y>=240)return;
        for(int i=0;i<count;i++)dma[i]=(colors[i]>>8)|(colors[i]<<8);
        if(esp_lcd_panel_draw_bitmap(panel,x,y,x+count,y+1,dma)!=ESP_OK ||
           xSemaphoreTake(done,pdMS_TO_TICKS(2000))!=pdTRUE)display_ok=false;
        if((y&15)==15)taskYIELD();
    }
} output;
}
smalltv::Display& board_display(){return output;}
bool board_init(){
    done=xSemaphoreCreateBinary();
    dma=static_cast<uint16_t*>(heap_caps_malloc(240*2,MALLOC_CAP_DMA));
    if(!done || !dma)return false;
    ledc_timer_config_t timer={};timer.speed_mode=LEDC_LOW_SPEED_MODE;
    timer.duty_resolution=LEDC_TIMER_8_BIT;timer.timer_num=LEDC_TIMER_0;timer.freq_hz=5000;timer.clk_cfg=LEDC_AUTO_CLK;
    if(ledc_timer_config(&timer)!=ESP_OK)return false;
    ledc_channel_config_t backlight={};backlight.gpio_num=5;backlight.speed_mode=LEDC_LOW_SPEED_MODE;
    backlight.channel=LEDC_CHANNEL_0;backlight.timer_sel=LEDC_TIMER_0;backlight.flags.output_invert=1;
    if(ledc_channel_config(&backlight)!=ESP_OK)return false;
    spi_bus_config_t bus={};bus.sclk_io_num=1;bus.mosi_io_num=2;bus.miso_io_num=-1;
    bus.quadwp_io_num=-1;bus.quadhd_io_num=-1;bus.max_transfer_sz=512;
    if(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO)!=ESP_OK)return false;
    esp_lcd_panel_io_spi_config_t config={};config.dc_gpio_num=0;config.cs_gpio_num=-1;
    config.pclk_hz=40000000;config.lcd_cmd_bits=8;config.lcd_param_bits=8;config.spi_mode=3;
    config.trans_queue_depth=1;config.on_color_trans_done=transferred;
    esp_lcd_panel_io_handle_t io;
    if(esp_lcd_new_panel_io_spi(static_cast<esp_lcd_spi_bus_handle_t>(SPI2_HOST),&config,&io)!=ESP_OK)return false;
    esp_lcd_panel_dev_config_t device={};device.reset_gpio_num=3;
    device.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB;device.bits_per_pixel=16;
    if(esp_lcd_new_panel_st7789(io,&device,&panel)!=ESP_OK)return false;
    if(esp_lcd_panel_reset(panel)!=ESP_OK || esp_lcd_panel_init(panel)!=ESP_OK)return false;
    if(esp_lcd_panel_disp_on_off(panel,true)!=ESP_OK)return false;
    return true;
}
void board_apply(const Settings& s){
    if(!display_ok)return;
    ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,s.brightness*255/100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
    esp_lcd_panel_invert_color(panel,s.invert);
    esp_lcd_panel_mirror(panel,s.rotate,s.rotate);
    esp_lcd_panel_set_gap(panel,0,s.rotate?80:0);
}
