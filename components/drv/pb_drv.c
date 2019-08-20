#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "led_drv.h"

#define     PB_FREQ_D		39	
#define     PB_FREQ_U		34 
#define     PB_OUT_EN		35
#define     PB_VOL_D		19	
#define     PB_VOL_U		18

#define     Bit_RESET		0
#define     Bit_SET		 	1	

#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INPUT_PIN_SEL ((1ULL<<PB_FREQ_D)  | (1ULL<<PB_FREQ_U) | (1ULL<<PB_OUT_EN)  | (1ULL<<PB_VOL_D) | (1ULL<<PB_VOL_U))

typedef struct
{	
    uint8_t 		freq_index;
    uint8_t 		volum_index;
    uint8_t 		out_en;
}pb_st;


static xQueueHandle gpio_evt_queue = NULL;
static pb_st pb_inst;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void pb_evt(uint16_t pin_id)
{
	switch (pin_id)
	{
		case PB_FREQ_D:
		{
			if(pb_inst.freq_index > 1)
				pb_inst.freq_index--;
			set_freq_led(pb_inst.freq_index);
			break;		
		}
		case PB_VOL_D:
		{
			if(pb_inst.volum_index > 1)
				pb_inst.volum_index--;
			set_vol_led(pb_inst.volum_index);
			break;		
		}
		case PB_FREQ_U:
		{
			if(pb_inst.freq_index < 4)
				pb_inst.freq_index++;
			set_freq_led(pb_inst.freq_index);
			break;		
		}
		case PB_VOL_U:
		{
			if(pb_inst.volum_index < 4)
				pb_inst.volum_index++;
			set_vol_led(pb_inst.volum_index);
			break;		
		}
		case PB_OUT_EN:
		{
			if(pb_inst.out_en == 0)
			{
				pb_inst.out_en = 1;
				pb_inst.freq_index = 1;
				pb_inst.volum_index = 1;
			}
			else
			{
				pb_inst.out_en = 0;
				pb_inst.freq_index = 0;
				pb_inst.volum_index = 0;
			}
			set_ind_led(0,pb_inst.out_en);
			set_vol_led(pb_inst.volum_index);
			set_freq_led(pb_inst.freq_index);
			break;		
		}
	}
}

void pb_cb(void)
{
    uint32_t io_num;
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) 
	{
        printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
		pb_evt(io_num);
    }
}

void pb_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19fasdf
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
	
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PB_FREQ_D, gpio_isr_handler, (void*) PB_FREQ_D);
    gpio_isr_handler_add(PB_FREQ_U, gpio_isr_handler, (void*) PB_FREQ_U);
    gpio_isr_handler_add(PB_OUT_EN, gpio_isr_handler, (void*) PB_OUT_EN);
    gpio_isr_handler_add(PB_VOL_D, gpio_isr_handler, (void*) PB_VOL_D);
    gpio_isr_handler_add(PB_VOL_U, gpio_isr_handler, (void*) PB_VOL_U);
	pb_inst.freq_index = 0;
	pb_inst.volum_index = 0;
	pb_inst.out_en = 0;
}



