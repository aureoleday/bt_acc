/* Console example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usr_sys_init.h"
#include "threads.h"
#include "cli_shell.h"

enum
{
	TEST_THREAD_PRIO=3,
	CLI_THREAD_PRIO,
	CMD_THREAD_PRIO,
	TCP_THREAD_PRIO,
	SPP_THREAD_PRIO,
	GEO_THREAD_PRIO,
	USR_MAX_PRIO
};

#define TEST_THREAD_STACK_SIZE 	3072
#define TCP_THREAD_STACK_SIZE 	4096
#define CLI_THREAD_STACK_SIZE 	3072
#define CMD_THREAD_STACK_SIZE 	3072
#define SPP_THREAD_STACK_SIZE 	2048
#define GEO_THREAD_STACK_SIZE 	3072

static void tasks_create(void)
{
    xTaskCreate(&test_thread,
                "Task_test",
				TEST_THREAD_STACK_SIZE,
                NULL,
				TEST_THREAD_PRIO,
				NULL);

    xTaskCreate(&cli_thread,
                "Task_cli",
				CLI_THREAD_STACK_SIZE,
                NULL,
				CLI_THREAD_PRIO,
				NULL);

    xTaskCreate(&cmd_thread,
                "Task_CMD",
				CMD_THREAD_STACK_SIZE,
                NULL,
				CMD_THREAD_PRIO,
				NULL);

//    xTaskCreate(&spp_thread,
//                "Task_spp",
//    			SPP_THREAD_STACK_SIZE,
//                NULL,
//    			SPP_THREAD_PRIO,
//    			NULL);

//	xTaskCreate(&tcp_thread,
//				"Task_tcp",
//				TCP_THREAD_STACK_SIZE,
//				NULL,
//				TCP_THREAD_PRIO,
//				NULL);

    xTaskCreate(&geo_thread,
                "Task_geo",
				GEO_THREAD_STACK_SIZE,
                NULL,
				GEO_THREAD_PRIO,
				NULL);
}

void app_main()
{
	usr_sys_init();
	tasks_create();
}
