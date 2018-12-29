/*
 * spi_drv.c
 *
 *  Created on: 2018Äê12ÔÂ27ÈÕ
 *      Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define DEV_GEO_RTX_SIZE    2048
#define DEV_GEO_FIFO_SIZE   256

typedef struct
{
    spi_device_handle_t     		spi_device_h;
    uint8_t                         txd[DEV_GEO_RTX_SIZE];
    uint8_t                         rxd[DEV_GEO_RTX_SIZE];
}spi_geo_device_st;

spi_geo_device_st spi_geo_dev_inst;

uint8_t adxl_wr_reg(uint8_t addr, uint8_t data)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    t.length=8*2;
    t.user=(void*)0;

    spi_geo_dev_inst.txd[0] = (addr<<1);

    spi_geo_dev_inst.txd[1] = data;

    t.tx_buffer=spi_geo_dev_inst.txd;

    esp_err_t ret = spi_device_polling_transmit(spi_geo_dev_inst.spi_device_h, &t);

    return ret;
}

uint8_t adxl_rd_reg(uint8_t addr, uint8_t * rx_buf, uint8_t cnt)
{
    uint8_t i;
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    t.length=8*(cnt+1);
    t.rx_buffer = rx_buf;

    spi_geo_dev_inst.txd[0] = (addr<<1)|0x01;

    for(i=0;i<cnt;i++)
    {
    	spi_geo_dev_inst.txd[1+i] = 0;
    }

    t.tx_buffer=spi_geo_dev_inst.txd;

    esp_err_t ret = spi_device_polling_transmit(spi_geo_dev_inst.spi_device_h, &t);

    return ret;
}

void adxl_init(void)
{
    esp_err_t ret;
//    spi_device_handle_t spi;
    spi_bus_config_t buscfg=
    {
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=64
    };
    spi_device_interface_config_t devcfg=
    {
        .clock_speed_hz=1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
//        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi_geo_dev_inst.spi_device_h);
    ESP_ERROR_CHECK(ret);

}

/** Arguments used by 'join' function */
static struct {
    struct arg_int *addr;
    struct arg_int *data;
    struct arg_end *end;
} adxl_args;

static int ard_reg(int argc, char **argv)
{
	uint8_t rx_buf[16];
	uint8_t i;

    int nerrors = arg_parse(argc, argv, (void**) &adxl_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, adxl_args.end, argv[0]);
        return 1;
    }

    adxl_rd_reg(adxl_args.addr->ival[0],
    			rx_buf,
    			adxl_args.data->ival[0]);

	i=0;
	for(i=0;i<adxl_args.data->ival[0];i++)
		printf("%d: %x\n",i+adxl_args.addr->ival[0],rx_buf[i+1]);
	return 0;
}

static int awr_reg(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &adxl_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, adxl_args.end, argv[0]);
        return 1;
    }

    adxl_wr_reg(adxl_args.addr->ival[0],
    			adxl_args.data->ival[0]);
	return 0;
}

static void register_adxl_rd()
{
	adxl_args.addr = arg_int1(NULL, NULL, "<a>", "reg base addr");
	adxl_args.data = arg_int1(NULL, NULL, "<c>", "read reg count");
	adxl_args.end = arg_end(2);
    const esp_console_cmd_t cmd = {
        .command = "ard_reg",
        .help = "Read adxl regs",
        .hint = NULL,
        .func = &ard_reg,
		.argtable = &adxl_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static void register_adxl_wr()
{
	adxl_args.addr = arg_int1(NULL, NULL, "<a>", "reg base addr");
	adxl_args.data = arg_int1(NULL, NULL, "<d>", "write data");
	adxl_args.end = arg_end(2);
    const esp_console_cmd_t cmd = {
        .command = "awr_reg",
        .help = "Write adxl regs",
        .hint = NULL,
        .func = &awr_reg,
		.argtable = &adxl_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void adxl_register(void)
{
	register_adxl_rd();
	register_adxl_wr();
}


