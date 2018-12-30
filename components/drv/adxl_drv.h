/*
 * adxl_drv.h
 *
 *  Created on: 2018Äê12ÔÂ27ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_DRV_ADXL_DRV_H_
#define COMPONENTS_DRV_ADXL_DRV_H_

#include <stdio.h>

typedef enum {
    DEVID_AD = 0x00,
    DEVID_MST = 0x01,
    PARTID = 0x02,
    REVID = 0x03,
    ASTATUS = 0x04,
    FIFO_ENTRIES = 0x05,
    TEMP2 = 0x06,
    TEMP1 = 0x07,
    XDATA3 = 0x08,
    XDATA2 = 0x09,
    XDATA1 = 0x0A,
    YDATA3 = 0x0B,
    YDATA2 = 0x0C,
    YDATA1 = 0x0D,
    ZDATA3 = 0x0E,
    ZDATA2 = 0x0F,
    ZDATA1 = 0x10,
    FIFO_DATA = 0x11,
    OFFSET_X_H = 0x1E,
    OFFSET_X_L = 0x1F,
    OFFSET_Y_H = 0x20,
    OFFSET_Y_L = 0x21,
    OFFSET_Z_H = 0x22,
    OFFSET_Z_L = 0x23,
    ACT_EN = 0x24,
    ACT_THRESH_H = 0x25,
    ACT_THRESH_L = 0x26,
    ACT_COUNT = 0x27,
    FILTER = 0x28,
    FIFO_SAMPLES = 0x29,
    INT_MAP = 0x2A,
    SYNC = 0x2B,
    RANGE = 0x2C,
    POWER_CTL = 0x2D,
    SELF_TEST = 0x2E,
    ARESET = 0x2F
} ADXL355_register_t;

void adxl_init(void);
void adxl_register(void);
uint8_t adxl_rd_reg(uint8_t addr, uint8_t * rx_buf, uint8_t cnt);
uint8_t adxl_wr_reg(uint8_t addr, uint8_t* data, uint8_t cnt);


#endif /* COMPONENTS_DRV_ADXL_DRV_H_ */
