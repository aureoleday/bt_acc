#include "sys_def.h" 
#include "reg_map_check.h"
#include "netif/ethernetif.h"
#include "global_var.h"
//#include "drv_adxl355.h"

//extern sys_reg_st g_sys;
//
uint16_t dhcp_trigger(uint32_t pram)
{  
//    usr_dhcp_action(pram);
    return 1;
}

uint16_t sys_reset_opt(uint32_t pram)
{  
//    NVIC_SystemReset();
    return 1;
}

uint16_t set_boot_opt(uint32_t pram)
{  
//    set_startup_flag((uint8_t)pram);
//    rt_kprintf("set boot opt: %d\n", pram);
    return 1;
}

uint16_t save_conf_opt(uint32_t pram)
{  
//    save_conf((uint8_t)pram);
//    rt_kprintf("set save opt: %d\n", pram);
    return 1;
}


uint16_t set_timestamp(uint32_t pram)
{
//    rt_device_t device;
//    rt_err_t ret = -RT_ERROR;
//
//    device = rt_device_find("rtc");
//    /* update to RTC device. */
//    ret = rt_device_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, &pram);
//
//    if(ret == RT_EOK)
//        return 1;
//    else
        return 0;
}

uint16_t plc_conf_opt(uint32_t pram)
{
//    uint8_t temp[4];
//    rt_err_t ret = -RT_ERROR;
//    temp[0] = (pram>>24)&0x000000ff;
//    temp[1] = (pram>>16)&0x000000ff;
//    temp[2] = (pram>>8)&0x000000ff;
//    temp[3] = pram&0x000000ff;
//
//    ret = plc_tx(temp,4);
//
//    if(ret == 4)
//        return 1;
//    else
        return 0;
}

uint16_t tcp_timer_opt(uint32_t pram)
{
//    extern rt_timer_t tm_tcp_repo;
//    uint32_t period;
//    period = pram*RT_TICK_PER_SECOND;
//    rt_timer_control(tm_tcp_repo,RT_TIMER_CTRL_SET_TIME,(void*)&period);
    return 1;
}


uint16_t geo_timer_opt(uint32_t pram)
{
//    extern rt_timer_t tm_geo_repo;
//    uint32_t period;
//    period = pram;
//    rt_timer_control(tm_geo_repo,RT_TIMER_CTRL_SET_TIME,(void*)&period);
    return 1;
}

uint16_t geo_filter_opt(uint32_t pram)
{
//    uint8_t data = pram&0x0ff;
//
//    adxl355_activate(0);
//    rt_thread_delay(1);
//    adxl_wr_reg(FILTER,&data,1);
//    rt_thread_delay(1);
//    adxl355_activate(1);
    
    return 1;
}

uint16_t geo_pwr_opt(uint32_t pram)
{
//    uint8_t enable = pram;
//    adxl355_activate(enable);
    return 1;
}

static void sys_reset(void)
{
//    NVIC_SystemReset();
}

