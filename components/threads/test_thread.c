#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "my_fft.h"

//#define MEOW_FFT_IMPLEMENTAION
//
//#include "meow_fft.h"

const float sine_wave[64] = { 	0.   ,  0.098,  0.195,  0.29 ,  0.383,  0.471,  0.556,  0.634,
								0.707,  0.773,  0.831,  0.882,  0.924,  0.957,  0.981,  0.995,
								1.   ,  0.995,  0.981,  0.957,  0.924,  0.882,  0.831,  0.773,
								0.707,  0.634,  0.556,  0.471,  0.383,  0.29 ,  0.195,  0.098,
								0.   , -0.098, -0.195, -0.29 , -0.383, -0.471, -0.556, -0.634,
							   -0.707, -0.773, -0.831, -0.882, -0.924, -0.957, -0.981, -0.995,
							   -1.   , -0.995, -0.981, -0.957, -0.924, -0.882, -0.831, -0.773,
							   -0.707, -0.634, -0.556, -0.471, -0.383, -0.29 , -0.195, -0.098};

void fft_test1(void)
{
	int i;
	unsigned          N   = 1024;
	float*            ind  = malloc(sizeof(float) * N);
	float*            outd  = malloc(sizeof(float) * N);

	for(i=0;i<N;i++)
	    	*(ind+i)= sine_wave[2*i%64];

	fft_init(N);
	fft_calc(ind,outd);
	for(i=0;i<N/2;i++)
	{
		if(*(outd+i)>0)
			printf("%d: %f \n",i,*(outd+i));
	}
}

void test_thread(void* param)
{
	vTaskDelay(5000 / portTICK_PERIOD_MS);
//	fft_test1();
	while(1)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

