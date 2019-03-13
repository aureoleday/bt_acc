/*
 * sys_init.c
 *
 *  Created on: 2018��12��29��
 *      Author: Administrator
 */
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adxl_drv.h"
#include "global_var.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "threads.h"

char ssid[24];
char pwd[24];
char slen;
char plen;

static void initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void usr_sys_init(void)
{
	extern sys_reg_st  g_sys;
	initialize_nvs();

	adxl_init();
	adxl_register();
	usr_spp_register();

	gvar_init();
	gvar_register();
//	get_wifi_info(g_sys.stat.wifi.ssid,g_sys.stat.wifi.pwd,&g_sys.stat.wifi.slen,&g_sys.stat.wifi.plen);
//	get_wifi_info(g_sys.stat.wifi.ssid,g_sys.stat.wifi.pwd,&g_sys.stat.wifi.slen,&g_sys.stat.wifi.plen);
//	printf(" saved ssid:%s,pwd:%s,s_len:%d,p_len:%d\n",g_sys.stat.wifi.ssid,g_sys.stat.wifi.pwd,g_sys.stat.wifi.slen,g_sys.stat.wifi.plen);
}
