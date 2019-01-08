/*
 * cmd_thread.c
 *
 *  Created on: 2019Äê1ÔÂ8ÈÕ
 *      Author: Administrator
 */

#include "sys_conf.h"
#include "cmd_resolve.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
  * @brief 	output control module components cooldown
	* @param  none
	* @retval none
  */
void cmd_thread(void* param)
{
    vTaskDelay(CMD_THREAD_DELAY);
	cmd_dev_init();
	while(1)
	{
		recv_frame_fsm();
		cmd_frame_resolve();
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

