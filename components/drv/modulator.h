/*
 * modulator.h
 *
 *  Created on: 2019��3��20��
 *      Author: Administrator
 */

#ifndef COMPONENTS_DRV_MODULATOR_H_
#define COMPONENTS_DRV_MODULATOR_H_

#include <stdio.h>

void mod_init(void);
void mod_en(uint8_t enable);
void mod_volum_mdf(uint8_t dir);
void mod_freq_mdf(uint8_t dir);

#endif /* COMPONENTS_DRV_MODULATOR_H_ */
