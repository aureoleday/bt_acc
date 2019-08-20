/*
 * led.c
 *
 *  Created on: 2019Äê5ÔÂ6ÈÕ
 *      Author: Administrator
 */
#include "driver/gpio.h"
#define     SR_D			12 
#define     SR_CLK          14 
#define     SR_SRCLK        26
#define     SR_NCLR         27	
#define     SR_OE_n	        13

#define		PIN_LED_PWR	 	21
#define		PIN_LED_STS		22
#define		PIN_LED_COM		23

#define     Bit_RESET		0
#define     Bit_SET		 	1	

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<SR_D)  | (1ULL<<SR_CLK) | (1ULL<<SR_NCLR)  | (1ULL<<SR_OE_n) | (1ULL<<PIN_LED_PWR) | (1ULL<<PIN_LED_STS)| (1ULL<<PIN_LED_COM))

static uint16_t shift_reg_data;

void led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19fasdf
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(SR_D, 0);
    gpio_set_level(SR_CLK, 0);
    gpio_set_level(SR_SRCLK, 0);
    gpio_set_level(SR_NCLR, 1);
    gpio_set_level(SR_OE_n, 1);
    gpio_set_level(PIN_LED_PWR, 1);
    gpio_set_level(PIN_LED_STS, 1);
    gpio_set_level(PIN_LED_COM, 1);
}


static void sr_bit_op(uint16_t pin_type, uint8_t action)
{
	gpio_set_level(pin_type, action);
}

static void sr_delay(uint16_t time)
{
    uint16_t i;
    for(i=0;i<time;i++)
    ;

}

static void sr_output(void)
{
    sr_bit_op(SR_CLK,Bit_RESET);
    sr_delay(2);
    sr_bit_op(SR_CLK,Bit_SET);
    sr_delay(2);
}

static void sr_update(uint16_t sdata)
{
    uint16_t i = 0;
    uint16_t update_reg = sdata;

    for(i=0;i<16;i++)
    {
        sr_bit_op(SR_SRCLK,Bit_RESET);
        sr_delay(1);
        if(update_reg&0x8000)
            sr_bit_op(SR_D,Bit_SET);
        else
            sr_bit_op(SR_D,Bit_RESET);
        sr_delay(1);
        update_reg = update_reg << 1;
        sr_bit_op(SR_SRCLK,Bit_SET);
        sr_delay(1);
    }
    shift_reg_data = sdata;
}


void set_bat_led(uint16_t bat_cnt)
{
	uint16_t bat_bm;
	bat_bm = 0x00ff>>bat_cnt;
    uint16_t sr_reg = shift_reg_data;
	if((sr_reg>>8) == bat_bm)
		return;
	else
	{
    	sr_reg = (sr_reg & 0x00ff) | (bat_bm << 8);
    	sr_update(sr_reg);
    	shift_reg_data = sr_reg;
    	sr_output();
	}
}

void set_vol_led(uint16_t vol_cnt)
{
	uint16_t vol_reg;
	vol_reg = (~(0x0001<<vol_cnt))&0x000f;
    uint16_t sr_reg = shift_reg_data;
    sr_reg = (sr_reg & 0xff0f) | (vol_reg << 4);
    sr_update(sr_reg);
    shift_reg_data = sr_reg;
    sr_output();
}

void set_freq_led(uint16_t freq_cnt)
{
	uint16_t freq_reg;
    uint16_t sr_reg = shift_reg_data;
	freq_reg = (~(0x0001<<freq_cnt))&0x000f;
    sr_reg = (sr_reg & 0xfff0) | freq_reg;
    sr_update(sr_reg);
    shift_reg_data = sr_reg;
    sr_output();
}

void set_ind_led(uint8_t led_type, uint8_t bit_action)
{
	if(led_type == 0)
	{
		gpio_set_level(PIN_LED_PWR, bit_action);
	}
	if(led_type == 1)
	{
		gpio_set_level(PIN_LED_STS, bit_action);
	}
	if(led_type == 2)
	{
		gpio_set_level(PIN_LED_COM, bit_action);
	}
}

