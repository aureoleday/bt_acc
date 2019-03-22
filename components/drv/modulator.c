/*
 * modulator.c
 *
 *  Created on: 2019Äê3ÔÂ13ÈÕ
 *      Author: Administrator
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define MOD_FREQ_INC 	12
#define MOD_FREQ_DEC 	14
#define MOD_PWR_INC 	27
#define MOD_PWR_DEC 	26
#define MOD_EN		 	25

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<MOD_FREQ_INC) | (1ULL<<MOD_FREQ_DEC) | (1ULL<<MOD_PWR_INC) | (1ULL<<MOD_PWR_DEC) | (1ULL<<MOD_EN))


void mod_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(MOD_FREQ_INC, 1);
    gpio_set_level(MOD_FREQ_DEC, 1);
    gpio_set_level(MOD_PWR_INC, 1);
    gpio_set_level(MOD_PWR_DEC, 1);
    gpio_set_level(MOD_EN, 1);
}

void mod_en(uint8_t enable)
{
	if(enable == 1)
	{
		gpio_set_level(MOD_EN, 1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_EN, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_EN, 1);
	}
}

void mod_volum_mdf(uint8_t dir)
{
	if(dir == 1)
	{
		gpio_set_level(MOD_PWR_INC, 1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_PWR_INC, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_PWR_INC, 1);
	}
	else if(dir == 0)
	{
		gpio_set_level(MOD_PWR_DEC, 1);
		vTaskDelay(100 / MOD_PWR_DEC);
		gpio_set_level(MOD_PWR_INC, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_PWR_DEC, 1);
	}
}

void mod_freq_mdf(uint8_t dir)
{
	if(dir == 1)
	{
		gpio_set_level(MOD_FREQ_INC, 1);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_FREQ_INC, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_FREQ_INC, 1);
	}
	else if(dir == 0)
	{
		gpio_set_level(MOD_FREQ_DEC, 1);
		vTaskDelay(100 / MOD_PWR_DEC);
		gpio_set_level(MOD_FREQ_DEC, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(MOD_FREQ_DEC, 1);
	}
}


