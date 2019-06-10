/*
 * geo_thread.c
 *
 *  Created on: 2019Äê1ÔÂ8ÈÕ
 *      Author: Administrator
 */
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_var.h"
#include "adxl_drv.h"
#include "bit_op.h"
#include "my_fft.h"

void geo_thread(void* param)
{
    extern sys_reg_st  g_sys;
    int16_t  err_no;
    static uint16_t e_cnt = 0;

    adxl355_reset();
    fft_init();

	while(1)
	{
		if(g_sys.conf.geo.enable == 1)
		{
			err_no = adxl355_scanfifo();

			vTaskDelay(g_sys.conf.geo.sample_period / portTICK_PERIOD_MS);
		}
		else
			vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

