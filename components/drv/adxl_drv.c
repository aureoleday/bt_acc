/*
 * spi_drv.c
 *
 *  Created on: 2018��12��27��
 *      Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "adxl_drv.h"
#include "fifo.h"
#include "my_fft.h"
#include "sys_conf.h"
#include "kfifo.h"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define DEV_GEO_RTX_SIZE    512
#define DEV_GEO_FIFO_SIZE   2048

static uint8_t 	rxd_temp[DEV_GEO_RTX_SIZE];
static uint8_t 	kf_buf[3][DEV_GEO_RTX_SIZE];
kfifo_t 		kf_x,kf_y,kf_z;

typedef struct
{
	uint16_t	max_level;
	uint16_t	level;
	uint16_t	stage;
    float		ibuf[3][1024];
    float		obuf[3][1024];
}fft_st;

typedef struct
{
    spi_device_handle_t     		spi_device_h;
    uint8_t                         txd[DEV_GEO_RTX_SIZE];
    uint8_t                         rxd[DEV_GEO_RTX_SIZE];
}spi_geo_device_st;

spi_geo_device_st spi_geo_dev_inst;
fft_st fft_inst;

static void kf_init(void)
{
	memset(&kf_x, 0, sizeof(kf_x));
	memset(&kf_y, 0, sizeof(kf_y));
	memset(&kf_z, 0, sizeof(kf_z));
	kfifo_init(&kf_x, (void *)kf_buf[0], sizeof(kf_buf[0]));
	kfifo_init(&kf_y, (void *)kf_buf[1], sizeof(kf_buf[1]));
	kfifo_init(&kf_z, (void *)kf_buf[2], sizeof(kf_buf[2]));
}

static int32_t decode(uint32_t din)
{
	uint32_t temp;
	temp = din>>4;
	if((temp&0x80000) != 0)
		temp |= 0xfff00000;
	return (int32_t)temp;
}

static int16_t fft_prep(uint32_t geo_data)
{
	int16_t ret = 1;
	extern sys_reg_st g_sys;
	if(fft_inst.max_level == 0)
	{
		if(g_sys.conf.fft.en == 1)
		{
			fft_inst.max_level = 1<<g_sys.conf.fft.n;
			fft_inst.stage = 0;
			fft_inst.level = 0;
		}
	}
	else
	{
		if(fft_inst.level >= fft_inst.max_level)
		{
			fft_new(fft_inst.max_level);
			fft_calc(fft_inst.ibuf[0],fft_inst.obuf[0]);
			fft_calc(fft_inst.ibuf[1],fft_inst.obuf[1]);
			fft_calc(fft_inst.ibuf[2],fft_inst.obuf[2]);
			printf("%f,%f,%f\n",*(fft_inst.obuf[0]+1),*(fft_inst.obuf[1]+2),*(fft_inst.obuf[2]+3));

			g_sys.conf.fft.en = 0;
			fft_inst.max_level = 0;
			fft_inst.stage = 0;
			fft_inst.level = 0;
			ret = 0;
			printf("fft complete\n");
		}
		else
		{
			if((fft_inst.level==0)&&(fft_inst.stage == 0))
			{
				if((geo_data&0x01) == 1)
				{
					fft_inst.ibuf[fft_inst.stage][fft_inst.level] = (float)decode(geo_data)*0.0000039;
					fft_inst.stage = 1;
				}
			}
			else
			{
				fft_inst.ibuf[fft_inst.stage][fft_inst.level] = (float)decode(geo_data)*0.0000039;
				if(fft_inst.stage >= 2)
				{
					fft_inst.stage = 0;
					fft_inst.level++;
				}
				else
				{
					fft_inst.stage++;
				}
			}
		}
	}
	return ret;
}

void geo_ds_init(void)
{
	kf_init();
}

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

//    spi_device_polling_transmit(spi_geo_dev_inst.spi_device_h, &t);
    spi_device_transmit(spi_geo_dev_inst.spi_device_h, &t);

    return *(rx_buf+1);
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
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
//        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    geo_ds_init();
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi_geo_dev_inst.spi_device_h);
    ESP_ERROR_CHECK(ret);

}

void adxl355_reset(void)
{
	adxl_wr_reg(ADXL_RESET,0x52);
}

static int16_t data_split(uint32_t din)
{
	static uint8_t stage = 0;  //0: idle;1: x;2:y;3:z;
	static uint32_t dbuf[3]={0,0,0};
	int16_t ret = 0;
	switch (stage)
	{
		case 0:
		{
			if(din & 0x1)
			{
				dbuf[0] = din;
				stage = 2;
			}
			else
			{
				stage = 0;
			}
			ret = 0;
			break;
		}
		case 1:
		{
			if(din & 0x1)
			{
				kfifo_in(&kf_x,&dbuf[0],sizeof(uint32_t));
				kfifo_in(&kf_y,&dbuf[1],sizeof(uint32_t));
				kfifo_in(&kf_z,&dbuf[2],sizeof(uint32_t));
				dbuf[0] = din;
				stage = 2;
				ret = 0;
			}
			else
			{
				stage = 0;
				ret = -1;
			}
			break;
		}
		case 2:
		{
			if(!(din & 0x1))
			{
				dbuf[1] = din;
				stage = 3;
				ret = 0;
			}
			else
			{
				stage = 0;
				ret = -1;
			}
			break;
		}
		case 3:
		{
			if(!(din & 0x1))
			{
				dbuf[2] = din;
				stage = 1;
				ret = 0;
			}
			else
			{
				stage = 0;
				ret = -1;
			}
			break;
		}
		default:
		{
			stage = 0;
			ret = -1;
			break;
		}
	}
	return ret;
}

void adxl355_scanfifo(void)
{
	extern sys_reg_st  g_sys;
    uint16_t i;
    uint16_t total_cnt;
    uint8_t  sample_cnt;
    uint32_t buf_temp;
    uint8_t status;

    status = adxl_rd_reg(ADXL_STATUS,rxd_temp,1);
    if((status&0x6) != 0)
    	printf("adxl_fifo fuov!\n");

    sample_cnt = adxl_rd_reg(ADXL_FIFO_ENTRIES,rxd_temp,1);

    total_cnt = sample_cnt*3*3;

    if(rxd_temp[1] > 0)
    {
        adxl_rd_reg(ADXL_FIFO_DATA, rxd_temp, total_cnt);
        for(i=0;i<sample_cnt;i++)
        {
            buf_temp = (rxd_temp[1+i*3]<<16)|(rxd_temp[2+i*3]<<8)|(rxd_temp[3+i*3]);
            if(data_split(buf_temp) != 0)
            	printf("geo missing code\n");
            if(g_sys.conf.fft.en == 1)
            	fft_prep(buf_temp);
        }
    }
}

static int adxl_info(int argc, char **argv)
{
	printf("Status\tFentry\tFilt\tTemp\tPwr#\n");
	printf("%x\t%x\t%x\t%x\t%x\n",
			adxl_rd_reg(ADXL_STATUS,rxd_temp,1),
			adxl_rd_reg(ADXL_FIFO_ENTRIES,rxd_temp,1),
			adxl_rd_reg(ADXL_FILTER,rxd_temp,1),
			adxl_rd_reg(ADXL_TEMP2,rxd_temp,1),
			adxl_rd_reg(ADXL_POWER_CTL,rxd_temp,1));
	return 0;
}

static int fft_info(int argc, char **argv)
{
	printf("Maxleve\tLevel\tStage#\n");
	printf("%d\t%d\t%d\n",
			fft_inst.max_level,fft_inst.level,fft_inst.stage);

	return 0;
}

static int kf_read(int argc, char **argv)
{
	float rx_buf[16];
	if(kfifo_out(&kf_x,rx_buf,3) == 0)
		printf("kfifo empty!\n");
	else
	{
		printf("%f,%f,%f\n",rx_buf[0],rx_buf[1],rx_buf[2]);
	}

	return 0;
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

static void register_adxl_info()
{
    const esp_console_cmd_t cmd = {
        .command = "adxl_info",
        .help = "Get adxl_dev infomation",
        .hint = NULL,
        .func = &adxl_info
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static void register_fft_info()
{
    const esp_console_cmd_t cmd = {
        .command = "fft_info",
        .help = "Get fft infomation",
        .hint = NULL,
        .func = &fft_info
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static void register_kf_read()
{
    const esp_console_cmd_t cmd = {
        .command = "kf_read",
        .help = "Read kfifo",
        .hint = NULL,
        .func = &kf_read
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void adxl_register(void)
{
	register_adxl_rd();
	register_adxl_wr();
	register_adxl_info();
	register_fft_info();
	register_kf_read();
}


