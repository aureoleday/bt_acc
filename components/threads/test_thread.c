#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "cmd_wifi.h"
#include <math.h>
#include "my_fft.h"
#include "kfifo.h"
#include "led_drv.h"
#include "global_var.h"
#include "bit_op.h"
#include "dac_drv.h"
#include "bat_drv.h"

void join_wifi(void)
{
	char ssid[24];
	char lcssid[24];
	char pwd[24];
	char lcpwd[24];
	int err;
    size_t slen,lslen,plen,lplen;
    wifi_config_t wifi_config = { 0 };

    err = get_wifi_info(ssid,lcssid,pwd,lcpwd,&slen,&lslen,&plen,&lplen);

    if(err != 0)
    {
    	return;
    }

    printf("saved ssid:%s,pwd:%s,s_len:%d,p_len:%d\n",ssid,pwd,slen,plen);
    printf("connecting to station: %s...\n",ssid);

    strncpy((char*) wifi_config.sta.ssid, ssid, slen-1);
    strncpy((char*) wifi_config.sta.password, pwd, plen-1);

    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}



void test_thread(void* param)
{
	extern sys_reg_st  g_sys;
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	dac_init(1);
	bat_init();
	if((g_sys.conf.gen.wifi_mode == 1)&&bit_op_get(g_sys.stat.gen.status_bm,GBM_WIFI) == 1)
	{
		join_wifi();
	}

	while(1)
	{
		toggle_led(0);
		bat_update();
		if(g_sys.conf.gen.dbg == 1)
		{
			printf("\tacc\t\tcur\n");
			printf("SNR:\t%0.3f\t\t%0.3f\n",g_sys.stat.gtz.acc_snr,g_sys.stat.gtz.ins_snr);
			printf("offset:\t%d\t\t%d\n",g_sys.stat.gtz.acc_offset,g_sys.stat.gtz.offset);
			printf("s_psd:\t%f\t%f\n",g_sys.stat.gtz.acc_signal_level,g_sys.stat.gtz.signal_level);
			printf("n_psd:\t%f\t%f\n",g_sys.stat.gtz.acc_noise_level,g_sys.stat.gtz.noise_level);
			printf("rank:\t%d\t\t%d\n\n",g_sys.stat.gtz.acc_rank,g_sys.stat.gtz.rank);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if(g_sys.conf.gen.restart == 9527)
			esp_restart();
	}
}

