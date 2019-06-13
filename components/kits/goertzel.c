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

#define FREQ_SPAN_MAX	65

typedef struct
{
	float 		coef[FREQ_SPAN_MAX];
	float 		q0[FREQ_SPAN_MAX];
	float 		q1[FREQ_SPAN_MAX];
	float 		q2[FREQ_SPAN_MAX];
	float		res[FREQ_SPAN_MAX];
	uint32_t 	icnt;
}gtz_st;

typedef struct
{
	float 		snr;
	float 		ma_snr;
	float 		signal_level;
	float 		noise_level;
	float		snr_queue[FREQ_SPAN_MAX];
	uint16_t 	rank;
}snr_st;

gtz_st gtz_inst;
snr_st snr_inst;

static float goertzel_coef(uint32_t target_freq, uint32_t sample_freq, uint32_t N)
{
	uint32_t k;
	float w;
	k = N*target_freq/sample_freq;
	w = (2*M_PI*k)/N;
	return 2*cosf(w);
}

inline static float window(uint32_t n, uint32_t ord)
{
	return (1-cosf(2*M_PI*n/(ord-1)))/2;
}


int compare(const void * a, const void * b)
{
	float fa =  *(float*)a;
	float fb =  *(float*)b;
	return (fa-fb)>0? -1:1;
}

static float gtz_snr(float* dbuf, uint16_t cnt, snr_st *snr_sptr)
{
	extern sys_reg_st  g_sys;
	float buf[FREQ_SPAN_MAX];
	float buf_2nd[4];
	float signal_psd=0.0;
	float noise_psd=0.0;
	float snr = 0.0;
	float acc_snr = 0.0;
//	float max_queue = 0.0;

	uint16_t i;
	uint16_t ind=0;
	uint16_t mid = cnt/2;

	for(i=0;i<cnt;i++)
		buf[i] = *(dbuf+i);

	qsort(buf,cnt,sizeof(float),compare);

	buf_2nd[0] = *(dbuf+mid-1);
	buf_2nd[1] = *(dbuf+mid);
	buf_2nd[2] = *(dbuf+mid+1);
	buf_2nd[3] = buf[2];

	qsort(buf_2nd,4,sizeof(float),compare);

	for(i=0;i<4;i++)
	{
		if(buf_2nd[i]==buf[2])
		{
			ind = i;
			break;
		}
	}

	snr_sptr->rank = ind;

	if(ind != 0)
	{
		for(i=0;i<ind;i++)
			signal_psd += buf[i]*buf[i];
		signal_psd = sqrtf(signal_psd);
	}
	else
		signal_psd = *(dbuf+mid);

	snr_sptr->signal_level = signal_psd;

	for(i=3;i<cnt;i++)
		noise_psd += buf[i];
	noise_psd /=(cnt-ind);
	snr_sptr->noise_level = noise_psd;

	snr = signal_psd/noise_psd;
	snr_sptr->snr = snr;

	for(i=1;i<g_sys.conf.gtz.snr_mav_cnt;i++)
	{
		snr_sptr->snr_queue[g_sys.conf.gtz.snr_mav_cnt-i] = snr_sptr->snr_queue[g_sys.conf.gtz.snr_mav_cnt-i-1];
		acc_snr += (snr_sptr->snr_queue[g_sys.conf.gtz.snr_mav_cnt-i]);
	}
	if(ind==0)
		snr_sptr->snr_queue[0]=snr/(4*g_sys.conf.gtz.snr_mav_cnt);
	else if(snr_sptr->signal_level < 0.00003)
		snr_sptr->snr_queue[0]=snr/(2*g_sys.conf.gtz.snr_mav_cnt);
	else
		snr_sptr->snr_queue[0]=snr/g_sys.conf.gtz.snr_mav_cnt;
	snr_sptr->ma_snr = (acc_snr + snr_sptr->snr_queue[0]);

//	if(g_sys.conf.gen.dbg == 1)
//	{
//		printf("snr:%f,rank:%d\n",snr,ind);
//		printf("origin buf:\n");
//		for(i=0;i<cnt;i++)
//		{
//			printf(" %f ",dbuf[i]);
//		}
//		printf("\n");
//
//		printf("sorted buf:\n");
//		for(i=0;i<cnt;i++)
//		{
//			printf(" %f ",buf[i]);
//		}
//		printf("\n");
//
//		printf("sort buf_2nd:\n");
//		for(i=0;i<4;i++)
//		{
//			printf(" %f ",buf_2nd[i]);
//		}
//		printf("\n");
//	}

	return snr;
}

int16_t goertzel_lfilt(float din)
{
	extern sys_reg_st  g_sys;
	float x = 0.0;
	int16_t ret = 0;
	uint32_t i;

	if(gtz_inst.icnt == 0)
	{
		for(i=0;i<(2*g_sys.conf.gtz.target_span+1);i++)
		{
			gtz_inst.coef[i] = goertzel_coef(g_sys.conf.gtz.target_freq-g_sys.conf.gtz.target_span+i,g_sys.conf.gtz.sample_freq, g_sys.conf.gtz.n);
		}
	}

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
		}
		g_sys.stat.gtz.snr = gtz_snr(gtz_inst.res,2*g_sys.conf.gtz.target_span+1, &snr_inst);
		g_sys.stat.gtz.rank = snr_inst.rank;
		g_sys.stat.gtz.signal_level = snr_inst.signal_level;
		g_sys.stat.gtz.noise_level = snr_inst.noise_level;
		g_sys.stat.gtz.ma_snr = snr_inst.ma_snr;
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

int32_t gtz_freq_bins(float* dst_buf, uint16_t *num)
{
	extern sys_reg_st  g_sys;
	uint16_t i;
	*num = 2*g_sys.conf.gtz.target_span+1;
	for(i=0;i<*num;i++)
		*(dst_buf+i) = gtz_inst.res[i];
	return 0;
}


