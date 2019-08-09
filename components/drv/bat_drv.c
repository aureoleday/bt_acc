#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sys_conf.h"

#define PIN_BAT_CHARG 	34
#define PIN_BAT_STDB	35
#define GPIO_INPUT_PIN_SEL ((1ULL<<PIN_BAT_CHARG) | (1ULL<<PIN_BAT_STDB))

#define DEFAULT_VREF    1100
#define MAV_MAX_CNT		128
static esp_adc_cal_characteristics_t *adc_chars;

typedef struct
{
	uint32_t	mav_buffer[MAV_MAX_CNT];
    uint32_t    mav_cnt;
	uint32_t	accum_sum;
	uint32_t	buffer_ff;
	uint32_t	bat_sts;
}bat_st;

static bat_st bat_inst;

static void bat_sts_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void adc_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

static uint32_t adc_get_raw(void)
{
    return adc1_get_raw(ADC1_CHANNEL_0);
}

static uint32_t adc_get_volt(void)
{
    uint32_t temp,volt;
    temp = adc1_get_raw(ADC1_CHANNEL_0);
    volt = esp_adc_cal_raw_to_voltage(temp, adc_chars);
    return volt;
}

static uint32_t bat_mav_calc(uint16_t mav_cnt_set)
{
	uint32_t bat_mav_volt = 0;
	uint32_t cur_volt = adc_get_volt();
	if(bat_inst.buffer_ff == 0)
	{
		bat_inst.mav_buffer[bat_inst.mav_cnt] = cur_volt; 
		bat_inst.accum_sum += bat_inst.mav_buffer[bat_inst.mav_cnt];
		bat_inst.mav_cnt ++;
		bat_mav_volt = bat_inst.accum_sum/bat_inst.mav_cnt;
		if(bat_inst.mav_cnt >= mav_cnt_set)
	    {
       	 	bat_inst.mav_cnt = 0;
        	bat_inst.buffer_ff = 1;
   		}
	}
	else
	{
		bat_inst.accum_sum -= bat_inst.mav_buffer[bat_inst.mav_cnt];
		bat_inst.mav_buffer[bat_inst.mav_cnt] = cur_volt;
		bat_inst.accum_sum += bat_inst.mav_buffer[bat_inst.mav_cnt]; 
		bat_inst.mav_cnt ++;
		if(bat_inst.mav_cnt >= mav_cnt_set)
       	 	bat_inst.mav_cnt = 0;
		bat_mav_volt = bat_inst.accum_sum/mav_cnt_set;
	}
	return bat_mav_volt*83/50;
}

static uint32_t bat_pwr_calc(uint32_t up_lim, uint32_t low_lim, uint32_t bat_volt)
{
	uint32_t res_pwr = 0;
	res_pwr = 100*(bat_volt-low_lim)/(up_lim-low_lim);
	if(res_pwr > 100)
		res_pwr = 100;
	return res_pwr;
}

static uint32_t get_bat_sts(void)
{
	uint32_t pin_sts=0;
	pin_sts = gpio_get_level(PIN_BAT_CHARG)|(gpio_get_level(PIN_BAT_STDB)<<1);
	return pin_sts; 
}

void bat_update(void)
{
	extern sys_reg_st g_sys;
	g_sys.stat.bat.adc_raw = bat_mav_calc(g_sys.conf.bat.mav_cnt);
	g_sys.stat.bat.pwr_val = bat_pwr_calc(g_sys.conf.bat.up_lim,g_sys.conf.bat.low_lim,g_sys.stat.bat.adc_raw);
	g_sys.stat.bat.pwr_sts = get_bat_sts();
}

void bat_init(void) 
{
	uint16_t i;
	for(i=0;i<MAV_MAX_CNT;i++)
	{
		bat_inst.mav_buffer[i] = 0;
	}
	bat_inst.accum_sum = 0;
	bat_inst.mav_cnt = 0;
	adc_init();
	bat_sts_init();
}

