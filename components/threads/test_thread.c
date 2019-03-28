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
#include "global_var.h"
#include "modulator.h"

//#define MEOW_FFT_IMPLEMENTAION
//
//#include "meow_fft.h"
//
//const float sine_wave[64] = { 	0.   ,  0.098,  0.195,  0.29 ,  0.383,  0.471,  0.556,  0.634,
//								0.707,  0.773,  0.831,  0.882,  0.924,  0.957,  0.981,  0.995,
//								1.   ,  0.995,  0.981,  0.957,  0.924,  0.882,  0.831,  0.773,
//								0.707,  0.634,  0.556,  0.471,  0.383,  0.29 ,  0.195,  0.098,
//								0.   , -0.098, -0.195, -0.29 , -0.383, -0.471, -0.556, -0.634,
//							   -0.707, -0.773, -0.831, -0.882, -0.924, -0.957, -0.981, -0.995,
//							   -1.   , -0.995, -0.981, -0.957, -0.924, -0.882, -0.831, -0.773,
//							   -0.707, -0.634, -0.556, -0.471, -0.383, -0.29 , -0.195, -0.098};
//
//void fft_test1(void)
//{
//	int i;
//	unsigned          N   = 1024;
//	float*            ind  = malloc(sizeof(float) * N);
//	float*            outd  = malloc(sizeof(float) * N);
//
//	for(i=0;i<N;i++)
//	    	*(ind+i)= sine_wave[2*i%64];
//
////	fft_init();
//	fft_new(N);
//	fft_calc(ind,outd);
//	for(i=0;i<N/2;i++)
//	{
//		if(*(outd+i)>0)
//			printf("%d: %f \n",i,*(outd+i));
//	}
//}


void join_wifi(void)
{
	char ssid[24];
	char lcssid[24];
	char pwd[24];
	char lcpwd[24];
    size_t slen,lslen,plen,lplen;
    wifi_config_t wifi_config = { 0 };

    get_wifi_info(ssid,lcssid,pwd,lcpwd,&slen,&lslen,&plen,&lplen);

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
	vTaskDelay(5000 / portTICK_PERIOD_MS);
//	fft_test1();
	if(g_sys.conf.gen.wifi_mode == 1)
		join_wifi();

	while(1)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if(g_sys.conf.gen.restart == 9527)
			esp_restart();
	}
}

