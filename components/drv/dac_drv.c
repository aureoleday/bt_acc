#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/adc.h"
#include "soc/rtc_periph.h"
#include "soc/sens_reg.h"
#include "esp_adc_cal.h"
#include "pb_drv.h"

typedef struct
{	
    uint16_t 		tbl_gap;
    uint16_t 		tim_cnt;
}dac_st;

static dac_st dac_inst;

static const uint16_t cos_lookup_tbl[256] = 
{255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 251, 250, 249,
 248, 247, 246, 245, 243, 242, 240, 239, 237, 236, 234, 232, 230, 228,
 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 201, 199, 196,
 193, 191, 188, 185, 182, 179, 176, 174, 171, 168, 165, 162, 159, 156,
 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112,
 109, 106, 103,  99,  96,  93,  90,  87,  84,  81,  79,  76,  73,  70,
  67,  64,  62,  59,  56,  54,  51,  49,  46,  44,  42,  39,  37,  35,
  33,  31,  29,  27,  25,  23,  21,  19,  18,  16,  15,  13,  12,  10,
   9,   8,   7,   6,   5,   4,   3,   3,   2,   1,   1,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   2,   3,   3,   4,
   5,   6,   7,   8,   9,  10,  12,  13,  15,  16,  18,  19,  21,  23,
  25,  27,  29,  31,  33,  35,  37,  39,  42,  44,  46,  49,  51,  54,
  56,  59,  62,  64,  67,  70,  73,  76,  79,  81,  84,  87,  90,  93,
  96,  99, 103, 106, 109, 112, 115, 118, 121, 124, 127, 131, 134, 137,
 140, 143, 146, 149, 152, 156, 159, 162, 165, 168, 171, 174, 176, 179,
 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 240, 242,
 243, 245, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255,
 255, 255, 255, 255};


esp_timer_handle_t dac_ptimer;

static void dac_timer_cb(void* arg);
const esp_timer_create_args_t periodic_timer_args = {
    .callback = &dac_timer_cb,
    /* name is optional, but may help identify the timer when debugging */
    .name = "dac_periodic"
};

static void dac_timer_cb(void* arg)
{
    static uint16_t reg_cnt = 0;
    dac_output_voltage(DAC_CHANNEL_1, cos_lookup_tbl[reg_cnt]>>get_atten());
    reg_cnt = (reg_cnt+dac_inst.tbl_gap)&0x00ff;
}

static void tim_init(void)
{
    dac_inst.tbl_gap = 24;
    dac_inst.tim_cnt = 200;


    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &dac_ptimer));
    /* The timer has been created but is not running yet */
    ESP_ERROR_CHECK(esp_timer_start_periodic(dac_ptimer, dac_inst.tim_cnt));
}

void dac_init(uint8_t channel)
{
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 0);
	tim_init();
}

void dac_volt(uint8_t volt)
{
	dac_output_voltage(1,volt);
}

static void dac_tim_rst(void)
{
    esp_timer_stop(dac_ptimer);
    esp_timer_delete(dac_ptimer);
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &dac_ptimer));
    /* The timer has been created but is not running yet */
    ESP_ERROR_CHECK(esp_timer_start_periodic(dac_ptimer, dac_inst.tim_cnt));
}

void update_tim_param(uint8_t freq_mod)
{
    switch (freq_mod)
    {
        case 0:
        {
            dac_inst.tbl_gap = 15;
            dac_inst.tim_cnt = 200;
            break;
        }
        case 1:
        {
            dac_inst.tbl_gap = 24;
            dac_inst.tim_cnt = 200;
            break;
        }
        case 2:
        {
            dac_inst.tbl_gap = 30;
            dac_inst.tim_cnt = 200;
            break;
        }
        case 3:
        {
            dac_inst.tbl_gap = 35;
            dac_inst.tim_cnt = 200;
            break;
        }
        case 4:
        {
            dac_inst.tbl_gap = 35;
            dac_inst.tim_cnt = 200;
            break;
        }
        default:
        {
            dac_inst.tbl_gap = 15;
            dac_inst.tim_cnt = 200;
            break;
        }
    }
    dac_tim_rst();
}

void drv_dac_en(uint8_t enable)
{
    if(enable == 1)
        dac_output_enable(DAC_CHANNEL_1);
    else
        dac_output_disable(DAC_CHANNEL_1);
}

