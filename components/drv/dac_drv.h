/*
 *  dac_drv.h 
 *
 *  Created on: 2019Äê5ÔÂ6ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_DRV_DAC_DRV_H_
#define COMPONENTS_DRV_DAC_DRV_H_

void dac_init(uint8_t channel);
void dac_volt(uint8_t volt);
void update_tim_param(uint8_t freq_mod);
void drv_dac_en(uint8_t enable);
//uint32_t adc_get_raw(void);
//uint32_t adc_get_volt(void);
#endif /* COMPONENTS_DRV_DAC_DRV_H_ */
