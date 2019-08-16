/*
 * led_drv.h
 *
 *  Created on: 2019��5��6��
 *      Author: Administrator
 */

#ifndef COMPONENTS_DRV_LED_DRV_H_
#define COMPONENTS_DRV_LED_DRV_H_

void led_init(void);
void set_bat_led(uint16_t bat_cnt);
void set_pwr_led(uint16_t pwr_cnt);
void set_freq_led(uint16_t freq_cnt);
void set_ind_led(uint8_t led_type, uint8_t bit_action);

#endif /* COMPONENTS_DRV_LED_DRV_H_ */
