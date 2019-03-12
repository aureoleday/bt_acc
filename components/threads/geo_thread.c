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

    adxl355_reset();
    fft_init();

	while(1)
	{
		if((g_sys.conf.geo.enable == 1)&&((bit_op_get(g_sys.stat.gen.status_bm,GBM_BT) == 1)||(bit_op_get(g_sys.stat.gen.status_bm,GBM_TCP) == 1)||(g_sys.conf.fft.en == 1)))
		{
			adxl355_scanfifo();
			vTaskDelay(g_sys.conf.geo.sample_period / portTICK_PERIOD_MS);
		}
		else
			vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

