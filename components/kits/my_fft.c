/*
 * my_fft.c
 *
 *  Created on: 2019Äê3ÔÂ7ÈÕ
 *      Author: Administrator
 */


#define MEOW_FFT_IMPLEMENTAION
#define _USE_MATH_DEFINES
#include "meow_fft.h"
#include <math.h>
#include <stdio.h>

typedef struct
{
	float*					inbuf;
	Meow_FFT_Complex*		out_dbuf_ptr;
	Meow_FFT_Workset_Real*	fft_sbuf_ptr;
	float*					win_buf_ptr;
	uint16_t				ord;
}my_fft_st;

my_fft_st my_fft_inst;

static void win_init(uint16_t ord);

void fft_init(uint16_t ord)
{
	size_t workset_bytes = 0;
	if((my_fft_inst.out_dbuf_ptr != NULL)||(my_fft_inst.fft_sbuf_ptr != NULL))
	{
		free(my_fft_inst.inbuf);
		free(my_fft_inst.out_dbuf_ptr);
		free(my_fft_inst.fft_sbuf_ptr);
		free(my_fft_inst.win_buf_ptr);
	}


	workset_bytes = meow_fft_generate_workset_real(ord, NULL);
	my_fft_inst.out_dbuf_ptr = malloc(sizeof(Meow_FFT_Complex) * ord);
	my_fft_inst.fft_sbuf_ptr = (Meow_FFT_Workset_Real*) malloc(workset_bytes);
	my_fft_inst.win_buf_ptr = (float*) malloc(sizeof(float) * ord);
	my_fft_inst.inbuf = (float*) malloc(sizeof(float) * ord);
	my_fft_inst.ord = ord;

	meow_fft_generate_workset_real(my_fft_inst.ord, my_fft_inst.fft_sbuf_ptr);
	win_init(ord);
}

static void win_init(uint16_t ord)
{
	for(int i=1;i<(ord+1);i++)
		my_fft_inst.win_buf_ptr[i] = (1-cos(2*M_PI*i/(ord+1)))/2;
}

void win_apply(float* inbuf, float* outbuf)
{
	for(int i=0;i<my_fft_inst.ord;i++)
		*(outbuf+i) = *(inbuf+i)*my_fft_inst.win_buf_ptr[i];
}

void fft_calc(float* input_dbuf,float* output_dbuf)
{
	win_apply(input_dbuf,my_fft_inst.inbuf);
	printf("win\n");
	meow_fft_real(my_fft_inst.fft_sbuf_ptr, my_fft_inst.inbuf, my_fft_inst.out_dbuf_ptr);
	printf("fft\n");
	for(int i=0;i<my_fft_inst.ord/2;i++)
	{
		*(output_dbuf+i) = sqrt(pow(my_fft_inst.out_dbuf_ptr[i].r,2) + pow(my_fft_inst.out_dbuf_ptr[i].j,2))/(my_fft_inst.ord/2);
	}
}





