#include "sys_def.h" 
#include "reg_map_check.h"
#include "global_var.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"


//extern sys_reg_st g_sys;
//
uint16_t dhcp_trigger(uint32_t pram)
{  
    //    usr_dhcp_action(pram);
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
    if(pram == 1)
    {
        save_conf("usr");
    }
    return 1;
}

uint16_t load_conf_opt(uint32_t pram)
{
    if(pram == 1)
    {
        load_conf("x");
        save_conf("usr");
    }
    return 1;
}

uint16_t set_timestamp(uint32_t pram)
{
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


uint16_t mod_en_opt(uint32_t pram)
{
    //    uint8_t data = pram&0x0ff;
    //
    //    mod_en(data);

    return 1;
}

uint16_t mod_volum_opt(uint32_t pram)
{
    //    uint8_t data = pram&0x0ff;
    //    uint8_t i;
    //
    //    switch (data)
    //    {
    //    	case 0:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_volum_mdf(0);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    	case 1:
    //    	{
    //   			mod_volum_mdf(0);
    //    		break;
    //    	}
    //    	case 2:
    //    	{
    //   			mod_volum_mdf(1);
    //    		break;
    //    	}
    //    	case 3:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_volum_mdf(1);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    	default:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_volum_mdf(0);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    }

    return 1;
}

uint16_t mod_freq_opt(uint32_t pram)
{
    //    uint8_t data = pram&0x0ff;
    //    uint8_t i;
    //
    //    switch (data)
    //    {
    //    	case 0:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_freq_mdf(0);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    	case 1:
    //    	{
    //    		mod_freq_mdf(0);
    //    		break;
    //    	}
    //    	case 2:
    //    	{
    //    		mod_freq_mdf(1);
    //    		break;
    //    	}
    //    	case 3:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_freq_mdf(1);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    	default:
    //    	{
    //    		for(i=0;i<4;i++)
    //    		{
    //    			mod_freq_mdf(0);
    //    			vTaskDelay(5 / portTICK_PERIOD_MS);
    //    		}
    //    		break;
    //    	}
    //    }

    return 1;
}

