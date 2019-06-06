/*
 * goertzel.h
 *
 *  Created on: 2019��6��6��
 *      Author: Administrator
 */

#ifndef COMPONENTS_KITS_GOERTZEL_H_
#define COMPONENTS_KITS_GOERTZEL_H_

float goertzel_calc(float* din);
int16_t goertzel_lfilt(float din);
void gtz_register(void);

#endif /* COMPONENTS_KITS_GOERTZEL_H_ */