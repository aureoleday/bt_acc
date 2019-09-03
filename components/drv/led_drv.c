/*
 * led.c
 *
 *  Created on: 2019Äê5ÔÂ6ÈÕ
 *      Author: Administrator
 */
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_console.h"
#include "driver/gpio.h"
#include "sys_conf.h"
#define     SR_D			12 
#define     SR_CLK          14 
#define     SR_SRCLK        26
#define     SR_NCLR         27	
#define     SR_OE_n	        13

#define		PIN_LED_PWR	 	21
#define		PIN_LED_STS		22
#define		PIN_LED_COM		23

#define     Bit_RESET		0
#define     Bit_SET		 	1	

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<SR_D)  | (1ULL<<SR_SRCLK)  | (1ULL<<SR_CLK) | (1ULL<<SR_NCLR)  | (1ULL<<SR_OE_n) | (1ULL<<PIN_LED_PWR) | (1ULL<<PIN_LED_STS)| (1ULL<<PIN_LED_COM))

static uint16_t shift_reg_data;

static void sr_bit_op(uint16_t pin_type, uint8_t action)
{
	gpio_set_level(pin_type, action);
}

static void sr_delay(uint16_t time)
{
    uint16_t i;
    for(i=0;i<time*10;i++)
        ;
}

static void sr_output(void)
{
    sr_bit_op(SR_CLK,Bit_RESET);
    sr_delay(2);
    sr_bit_op(SR_CLK,Bit_SET);
    sr_delay(2);
}

static void sr_update(uint16_t sdata)
{
    uint16_t i = 0;
    uint16_t update_reg = sdata;

    for(i=0;i<16;i++)
    {
        sr_bit_op(SR_SRCLK,Bit_RESET);
        //sr_delay(1);
        if(update_reg&0x8000)
            sr_bit_op(SR_D,Bit_SET);
        else
            sr_bit_op(SR_D,Bit_RESET);
        sr_delay(1);
        update_reg = update_reg << 1;
        sr_bit_op(SR_SRCLK,Bit_SET);
        sr_delay(1);
    }
    shift_reg_data = sdata;
}


void set_bat_led(uint16_t bat_cnt)
{
	uint16_t bat_bm;
    uint16_t sr_reg = shift_reg_data;
	bat_bm = (0x00ff<<bat_cnt)&0x00ff;
	if((sr_reg&0x00ff) == bat_bm)
		return;
	else
	{
    	sr_reg = (sr_reg & 0xff00) | bat_bm;
    	sr_update(sr_reg);
    	shift_reg_data = sr_reg;
    	sr_output();
	}
}

void set_vol_led(uint16_t vol_cnt)
{
	uint16_t vol_reg;
    uint16_t sr_reg = shift_reg_data;
	vol_reg = (~(0x0001<<vol_cnt))&0x000f;
    sr_reg = (sr_reg & 0x0fff) | (vol_reg << 12);
    sr_update(sr_reg);
    shift_reg_data = sr_reg;
    sr_output();
}

void set_freq_led(uint16_t freq_cnt)
{
	uint16_t freq_reg;
    uint16_t sr_reg = shift_reg_data;
	freq_reg = (~(0x0001<<freq_cnt))&0x000f;
    sr_reg = (sr_reg & 0xf0ff) | (freq_reg<<8);
    sr_update(sr_reg);
    shift_reg_data = sr_reg;
    sr_output();
}

void set_ind_led(uint8_t led_type, uint8_t bit_action)
{
	if(led_type == 0)
	{
		gpio_set_level(PIN_LED_PWR, bit_action);
	}
	if(led_type == 1)
	{
		gpio_set_level(PIN_LED_STS, bit_action);
	}
	if(led_type == 2)
	{
		gpio_set_level(PIN_LED_COM, bit_action);
	}
}

void led_en(uint16_t enable)
{
    extern sys_reg_st  g_sys; 															
    if(enable != 0)
    {
        set_freq_led(g_sys.stat.pb.freq_index);
        set_vol_led(g_sys.stat.pb.volum_index);
    }
    else
    {
        set_freq_led(4);
        set_vol_led(4);
    }
}

/** Arguments used by 'mdns' function */
static struct {
    struct arg_int *dir;
    struct arg_end *end;
} led_mod_args;

static int bat_mod(int argc, char **argv)
{

    int nerrors = arg_parse(argc, argv, (void**) &led_mod_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, led_mod_args.end, argv[0]);
        return 1;
    }
    set_bat_led(led_mod_args.dir->ival[0]);

    return 0;
}

static int vol_mod(int argc, char **argv)
{

    int nerrors = arg_parse(argc, argv, (void**) &led_mod_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, led_mod_args.end, argv[0]);
        return 1;
    }
    set_vol_led(led_mod_args.dir->ival[0]);

    return 0;
}

static int freq_mod(int argc, char **argv)
{

    int nerrors = arg_parse(argc, argv, (void**) &led_mod_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, led_mod_args.end, argv[0]);
        return 1;
    }
    set_freq_led(led_mod_args.dir->ival[0]);

    return 0;
}

static int led_sts_info(int argc, char **argv)
{
    esp_err_t err = 0;
    printf("shiff reg:%x\n",shift_reg_data);
    return err;
}

static void register_bat_mod()
{
    led_mod_args.dir = arg_int1(NULL, NULL, "<a>", "bat change");
    led_mod_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
            .command = "bat_mod",
            .help = "Change bat led status",
            .hint = NULL,
            .func = &bat_mod,
            .argtable = &led_mod_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static void register_vol_mod()
{
    led_mod_args.dir = arg_int1(NULL, NULL, "<a>", "volum change");
    led_mod_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
            .command = "vol_mod",
            .help = "Change volum led status",
            .hint = NULL,
            .func = &vol_mod,
            .argtable = &led_mod_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static void register_freq_mod()
{
    led_mod_args.dir = arg_int1(NULL, NULL, "<a>", "freq change");
    led_mod_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
            .command = "freq_mod",
            .help = "Change freq led status",
            .hint = NULL,
            .func = &freq_mod,
            .argtable = &led_mod_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void register_led_sts_info()
{
    const esp_console_cmd_t cmd = {
            .command = "led_sts_info",
            .help = "print led sts info",
            .hint = NULL,
            .func = &led_sts_info
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19fasdf
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(SR_D, 1);
    gpio_set_level(SR_CLK, 0);
    gpio_set_level(SR_SRCLK, 0);
    gpio_set_level(SR_NCLR, 1);
    gpio_set_level(SR_OE_n, 0);
    gpio_set_level(PIN_LED_PWR, 1);
    gpio_set_level(PIN_LED_STS, 1);
    gpio_set_level(PIN_LED_COM, 1);

	vTaskDelay(10 / portTICK_PERIOD_MS);

    shift_reg_data = 0xffff;

    led_en(0);
    set_bat_led(0);
}

void led_register(void)
{
    register_led_sts_info();
    register_bat_mod();
    register_vol_mod();
    register_freq_mod();
}

