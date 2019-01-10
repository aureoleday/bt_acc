#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void test_thread(void* param)
{
	while(1)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

