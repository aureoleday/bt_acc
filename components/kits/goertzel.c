/*
 * goertzel.c
 *
 *  Created on: 2019Äê6ÔÂ6ÈÕ
 *      Author: Administrator
 */
//#include "esp_system.h"
//#include "freertos/FreeRTOS.h"
//#include "esp_console.h"
#include <stdlib.h>     /* qsort */
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


int compare(const void * a, const void * b)
{
	float fa =  *(float*)a;
	float fb =  *(float*)b;
	return (fa-fb)>0? -1:1;
}

static float gtz_snr(float* dbuf, uint16_t cnt)
{
	float buf[16];
	float buf_2nd[4];
	float signal_psd=0.0;
	float noise_psd=0.0;
	float snr = 0.0;

	uint16_t i;
	int16_t ind=0;

	for(i=0;i<cnt;i++)
		buf[i] = *(dbuf+i);

	qsort(buf,cnt,sizeof(float),compare);

	buf_2nd[0] = *(dbuf-1);
	buf_2nd[1] = *(dbuf);
	buf_2nd[2] = *(dbuf+1);
	buf_2nd[3] = buf[2];

	qsort(buf_2nd,4,sizeof(float),compare);

	for(i=0;i<4;i++)
	{
		if(buf_2nd[i]<=buf[2])
		{
			ind = i;
			break;
		}
	}

	if(ind != 0)
	{
		for(i=0;i<ind;i++)
			signal_psd += buf[i];
		signal_psd /=ind;

		for(i=ind;i<cnt;i++)
			noise_psd += buf[i];
		noise_psd /=(cnt-ind);

		snr = signal_psd/noise_psd;
	}

	return snr;
}



int16_t goertzel_lfilt(float din)
{
	extern sys_reg_st  g_sys;
	int16_t ret = 0;

	uint32_t i;

	if(gtz_inst.icnt == 0)
	{
		for(i=0;i<(2*g_sys.conf.gtz.target_span+1);i++)
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
			gtz_inst.res[i] = sqrtf(gtz_inst.q1[i]*gtz_inst.q1[i] + gtz_inst.q2[i]*gtz_inst.q2[i] - gtz_inst.q1[i]*gtz_inst.q2[i]*gtz_inst.coef[i])*2/g_sys.conf.gtz.n;
			gtz_inst.q0[i] = 0.0;
			gtz_inst.q1[i] = 0.0;
			gtz_inst.q2[i] = 0.0;
			//sort list
			if(i==g_sys.conf.gtz.target_span)
				g_sys.stat.gtz.freq_bar[0] = gtz_inst.res[i];
			else if(i<g_sys.conf.gtz.target_span)
				g_sys.stat.gtz.freq_bar[2*i+1] = gtz_inst.res[i];
			else
				g_sys.stat.gtz.freq_bar[2*(i-g_sys.conf.gtz.target_span)] = gtz_inst.res[i];
		}
		g_sys.stat.gtz.snr = gtz_snr(gtz_inst.res,2*g_sys.conf.gtz.target_span+1);
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




