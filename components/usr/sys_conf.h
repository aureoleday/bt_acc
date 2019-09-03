#ifndef __SYS_CONF
#define	__SYS_CONF
#include "sys_def.h"

//application delay
#define		CMD_THREAD_DELAY            400
#define		WS_THREAD_DELAY           	500
#define		BKG_THREAD_DELAY            700
#define		GEO_THREAD_DELAY            800
#define		TCPCLIENT_THREAD_DELAY      600

typedef struct
{	
    uint32_t 		wifi_mode;
    uint32_t 		restart;
    uint32_t 		dbg;
}conf_gen_st;


typedef struct
{
    uint32_t    mav_cnt;
    uint32_t    up_lim;
    uint32_t    low_lim;
}conf_bat_st;

typedef struct
{
    uint32_t    tcp_en;
    uint32_t    tcp_period;
    uint32_t    http_en;
}conf_eth_st;

typedef struct
{
    uint32_t    enable;
    uint32_t	mod_freq_off;
    uint32_t	symbol_period;
    uint32_t    volum;
    uint32_t    freq;
    uint32_t    setup_time;
    uint32_t    hold_time;
}conf_mod_st;

typedef struct
{
    conf_gen_st gen;
    conf_eth_st eth;
    conf_mod_st mod;
	conf_bat_st bat;
}config_st;


typedef struct
{	    
    uint32_t    status_reg_num;
    uint32_t    config_reg_num;
    uint32_t    software_ver;
    uint32_t    hardware_ver;
    uint32_t    status_bm;
}stat_gen_st;

typedef struct
{
    uint32_t    serial_no;
    uint32_t    man_date;
    uint32_t    dev_type;
}stat_man_st;

typedef struct
{	
    uint32_t 	freq_index;
    uint32_t 	volum_index;
    uint32_t 	out_en;
}stat_pb_st;


typedef struct
{
    uint32_t   	adc_raw;
    uint32_t   	pwr_val;
    uint32_t   	pwr_sts;
}stat_bat_st;

typedef struct
{
    stat_gen_st 	gen;
    stat_man_st 	man;
    stat_bat_st		bat;
    stat_pb_st		pb;
}status_st;

typedef struct
{
    config_st   conf;
    status_st   stat;
}sys_reg_st;

typedef struct 
{
    uint16_t 	id;
    uint32_t*	reg_ptr;
    int32_t	    min;
    uint32_t	max;
    uint32_t	dft;
    uint8_t	    type;                     //0:RW, 1:WO
    uint16_t    (*chk_ptr)(uint32_t pram);
}conf_reg_map_st;

typedef struct 
{
    uint16_t 	id;
    uint32_t*	reg_ptr;
    uint32_t  	dft;
}sts_reg_map_st;

uint16_t sys_global_var_init(void);

#endif //	__SYS_CONF




