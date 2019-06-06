/*
 * goertzel.c
 *
 *  Created on: 2019Äê6ÔÂ6ÈÕ
 *      Author: Administrator
 */
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "esp_console.h"
#include "global_var.h"
#include <math.h>

typedef struct
{
	float 		coef[32];
	float 		q0[32];
	float 		q1[32];
	float 		q2[32];
	float		res[32];
	uint32_t 	icnt;
}gtz_st;

gtz_st gtz_inst;

static float goertzel_coef(uint32_t target_freq, uint32_t sample_freq, uint32_t N)
{
	uint32_t k;
	float w;
	k = N*target_freq/sample_freq;
	w = (2*M_PI*k)/N;
	return 2*cosf(w);
}

static float window(uint32_t n, uint32_t ord)
{
	return (1-cosf(2*M_PI*n/(ord-1)))/2;
}

int16_t goertzel_lfilt(float din)
{
	extern sys_reg_st  g_sys;
	int16_t ret = 0;

	uint32_t i;

	if(gtz_inst.icnt == 0)
	{
		for(i=0;i<(2*g_sys.conf.gtz.target_span);i++)
		{
			gtz_inst.coef[i] = goertzel_coef(g_sys.conf.gtz.target_freq-g_sys.conf.gtz.target_span+i,g_sys.conf.gtz.sample_freq, g_sys.conf.gtz.n);
		}
	}

	float x = 0.0;

	if(gtz_inst.icnt<g_sys.conf.gtz.n)
	{
		x = din * window(gtz_inst.icnt,g_sys.conf.gtz.n);
		for(i=0;i<(2*g_sys.conf.gtz.target_span+1);i++)
		{
			gtz_inst.q0[i] = gtz_inst.coef[i] * gtz_inst.q1[i] - gtz_inst.q2[i] + x;
			gtz_inst.q2[i] = gtz_inst.q1[i];
			gtz_inst.q1[i] = gtz_inst.q0[i];
		}
		gtz_inst.icnt++;
		ret = 0;
	}

	if(gtz_inst.icnt>=g_sys.conf.gtz.n)
	{
		for(i=0;i<(2*g_sys.conf.gtz.target_span+1);i++)
		{
			gtz_inst.res[i] = sqrtf((gtz_inst.q1[i]*gtz_inst.q1[i] + gtz_inst.q2[i]*gtz_inst.q2[i] - gtz_inst.q1[i]*gtz_inst.q2[i]*gtz_inst.coef[i])*2/g_sys.conf.gtz.n);
			gtz_inst.q0[i] = 0.0;
			gtz_inst.q1[i] = 0.0;
			gtz_inst.q2[i] = 0.0;
			printf("\t%f\t",gtz_inst.res[i]);
		}
		printf("\n\n");
		gtz_inst.icnt = 0;
		ret = 1;
	}
	return ret;
}

float goertzel_calc(float* din)
{
	extern sys_reg_st  g_sys;

	float coef = goertzel_coef(g_sys.conf.gtz.target_freq,g_sys.conf.gtz.sample_freq, g_sys.conf.gtz.n);
	float q0 = 0.0;
	float q1 = 0.0;
	float q2 = 0.0;

	uint32_t i=0;
	float x = 0.0;

	for(i=0;i<g_sys.conf.gtz.n;i++)
	{
		x = *(din+i) * window(i,g_sys.conf.gtz.n);
		q0 = coef * q1 - q2 + x;
		q2 = q1;
		q1 = q0;
	}
	return sqrtf((q1*q1 + q2*q2 - q1*q2*coef)*2/g_sys.conf.gtz.n);
}

//static int cmd_gtz_info(int argc, char **argv)
//{
//	extern sys_reg_st  g_sys;
//	float coef;
//	coef = goertzel_coef(g_sys.conf.gtz.target_freq,g_sys.conf.gtz.sample_freq, g_sys.conf.gtz.n);
//	printf("goertzel coef:%f\n", coef);
//	return 0;
//}
//
//static void register_gtz_cal(void)
//{
//    const esp_console_cmd_t cmd = {
//        .command = "calc gtz",
//        .help = "Get gtz infomation",
//        .hint = NULL,
//        .func = &cmd_gtz_info
//    };
//    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
//}

void gtz_register(void)
{
//	register_gtz_cal();
}



