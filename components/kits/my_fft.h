/*
 * my_fft.h
 *
 *  Created on: 2019��3��7��
 *      Author: Administrator
 */


#ifndef COMPONENTS_KITS_MY_FFT_H_
#define COMPONENTS_KITS_MY_FFT_H_

#include "sys_def.h"

void fft_init(uint16_t ord);
void fft_calc(float* input_dbuf,float* output_dbuf);

#endif /* COMPONENTS_KITS_MY_FFT_H_ */
