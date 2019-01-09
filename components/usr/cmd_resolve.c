#include "sys_def.h"
#include "global_var.h"
#include "bit_op.h"
#include "fifo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
//cpad err code
enum
{
    CMD_ERR_NOERR = 0,
    CMD_ERR_ADDR_OR,
    CMD_ERR_DATA_OR,
    CMD_ERR_WR_OR,
    CMD_ERR_CONFLICT_OR,
    CMD_RDFIFO_EMPTY,
    CMD_WRFIFO_FULL,
    CMD_NOT_READY,
    CMD_ERR_UNKNOWN
};

enum
{
    FRAME_SYNC_POS = 0,
    FRAME_C_ATL_POS,
    FRAME_D_AL_POS,
    FRAME_D_PL_POS
};


#define CMD_FRAME_OVSIZE                 	  3

#define CMD_RTX_BUF_DEPTH 					  512
#define CMD_FSM_TIMEOUT	 					  2

#define CMD_FRAME_TAG_M_SYNC 			      0x1bdf9bdf
#define CMD_FRAME_TAG_S_SYNC 			      0x9bdf1bdf

#define CMD_FRAME_FSM_SYNC				      0x01
#define CMD_FRAME_FSM_TL					      0x02
#define CMD_FRAME_FSM_DATA					    0x04


#define CMD_RD_REG        0x0001
#define CMD_WR_REG        0x0002
#define CMD_RD_SER        0x0003
#define CMD_WR_SER        0x0004
#define CMD_RP_PKG        0x0080
#define CMD_RP_GEO        0x0100

fifo32_cb_td cmd_rx_fifo;
fifo32_cb_td cmd_tx_fifo;
esp_timer_handle_t geo_timer;

typedef struct
{
	uint32_t tx_buf[CMD_RTX_BUF_DEPTH];
    uint32_t rx_buf[CMD_RTX_BUF_DEPTH];
	uint16_t tx_cnt;
	uint16_t tx_cmd;
    uint8_t  tx_errcode;
    uint16_t rx_cnt;
    uint16_t rx_tag;
    uint16_t rtx_timeout;
    uint8_t cmd_fsm_cstate;
}cmd_reg_st;

static cmd_reg_st cmd_reg_inst;


/**
  * @brief  cmd rtx buffer initialization 
	* @param  none
  * @retval none
  */
static void cmd_buf_init(void)
{
    uint16_t i;
    //tx buffer initialization
    for(i=0;i<CMD_RTX_BUF_DEPTH;i++)
    {
        cmd_reg_inst.tx_buf[i] = 0;
    }
    for(i=0;i<CMD_RTX_BUF_DEPTH;i++)
    {
    	cmd_reg_inst.rx_buf[i] = 0;
    }
    cmd_reg_inst.tx_buf[0] = CMD_FRAME_TAG_S_SYNC;
    cmd_reg_inst.tx_cnt = 0;
    cmd_reg_inst.tx_cmd = 0;

    cmd_reg_inst.rx_cnt = 0;
    cmd_reg_inst.rx_tag = 0;
    cmd_reg_inst.rtx_timeout = 0;
    cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;

    //rx fifo initialization
    fifo32_init(&cmd_rx_fifo,1,CMD_RTX_BUF_DEPTH);
    fifo32_init(&cmd_tx_fifo,1,CMD_RTX_BUF_DEPTH);
}


/**
  * @brief  digital input sample interval timeout callback function, calls di_read() each time to update di buffer queue
  * @param  none
  * @retval none
  */
//static void cmd_timeout(void* parameter)
//{
//    uint16_t report_data(void);
//    report_data();
//}

static void geo_timeout(void* arg)
{
    uint16_t report_geo_data(void);
    report_geo_data();
}

/**
  * @brief   sample interval timer initialization, expires in 6 miliseconds pieriod
  * @param  none
  * @retval none
  */
//rt_timer_t tm_tcp_repo;
//static uint16_t	cmd_timer_init(void)
//{
//    extern sys_reg_st  g_sys;
////		rt_timer_t tm_tcp_repo;
//		tm_tcp_repo = rt_timer_create("tm_tcp_repo",
//									cmd_timeout,
//									RT_NULL,
//									(g_sys.conf.eth.tcp_period*RT_TICK_PER_SECOND),
//									RT_TIMER_FLAG_PERIODIC);
//		rt_timer_start(tm_tcp_repo);
//		return 1;
//}
//

static uint16_t	geo_timer_init(void)
{
	extern sys_reg_st g_sys;
	const esp_timer_create_args_t geo_timer_args = {
			.callback = &geo_timeout,
			/* name is optional, but may help identify the timer when debugging */
			.name = "periodic"
	};

	ESP_ERROR_CHECK(esp_timer_create(&geo_timer_args, &geo_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(geo_timer, g_sys.conf.geo.pkg_period*1000));
	return 0;
}


/**
  * @brief  cmd uart send interface
	* @param  tx_buf_ptr: tx src buffer pointer
						tx_cnt: tx src buffer transmitt count
  * @retval none
  */
void cmd_dev_init(void)
{
	cmd_buf_init();
//    cmd_timer_init();
    geo_timer_init();
}

/**
  * @brief  cmd uart send interface
	* @param  tx_buf_ptr: tx src buffer pointer
						tx_cnt: tx src buffer transmitt count
  * @retval none
  */
void cmd_uart_send_fifo(void) {
	uint32_t tx_data;

	if (is_fifo32_empty(&cmd_tx_fifo) == 0)
	{
		fifo32_pop(&cmd_tx_fifo, &tx_data);
	}
}

/**
  * @brief  cmd recieve frame checksum
	* @param  none
  * @retval 
			`0: checksum ok
			`1:	checksum fail
  */
static uint32_t frame_checksum(void) {
	uint32_t res, i;
	res = 0;
	for (i = 0; i < cmd_reg_inst.rx_cnt; i++)
	{
		res ^= cmd_reg_inst.rx_buf[i];
	}
	if (res == 0)
		return 1;
	else
		return 0;
}

/**
  * @brief  cmd transmmite frame checksum
	* @param  data_ptr: data buffer pointer whose checksum is to be caculated
						data_num: number of data to be caculated
  * @retval caculated checksum
  */
static uint32_t frame_checksum_gen(uint32_t* data_ptr, uint16_t data_num) {
	uint32_t res, i;
	res = 0;
	for (i = 0; i < data_num; i++)
	{
		res ^= *(data_ptr + i);
	}
	return res;
}

/**
  * @brief  cmd recieve frame finite state machine
	* @param  none
  * @retval 
			`0: cmd frame recieve ok
			`1:	cmd frame recieve ng
  */
uint16_t cmd_get_comm_sts(void)
{
	return cmd_reg_inst.rtx_timeout;
}

uint8_t cmd_get_rx_fsm(void)
{
	return cmd_reg_inst.cmd_fsm_cstate;
}

/**
  * @brief  cmd recieve frame finite state machine
	* @param  none
  * @retval 
			`0: cmd frame recieve ok
			`1:	cmd frame recieve ng
  */
uint16_t cmd_frame_recv(void)
{
	if (cmd_reg_inst.rx_tag == 1)//if there is already an unprocessed frame in the rx buffer, quit new frame recieving
			{
		return 1;
	} else {
		return 0;
	}
}


/**
  * @brief  cmd send response frame
	* @param  none
  * @retval none
  */
static void cmd_response(void)
{
	uint32_t i,check_sum;

	cmd_reg_inst.tx_buf[FRAME_SYNC_POS] = CMD_FRAME_TAG_S_SYNC;

	cmd_reg_inst.tx_buf[FRAME_C_ATL_POS] = (cmd_reg_inst.tx_errcode<<24)|(cmd_reg_inst.tx_cmd<<16)|cmd_reg_inst.tx_cnt;
	
	check_sum = frame_checksum_gen(&cmd_reg_inst.tx_buf[0],(cmd_reg_inst.tx_cnt+CMD_FRAME_OVSIZE-1));		//response frame checksum caculate
	cmd_reg_inst.tx_buf[cmd_reg_inst.tx_cnt+CMD_FRAME_OVSIZE-1] = check_sum;
	for(i=0;i<(cmd_reg_inst.tx_cnt+CMD_FRAME_OVSIZE);i++)																															//fifo test
	{
		fifo32_push(&cmd_tx_fifo,&cmd_reg_inst.tx_buf[i]);
	}
}

/**
  * @brief  cmd command write reg operation
	* @param  none
  * @retval 
		`CMD_ERR_NOERR			 : operation OK
		`CMD_ERR_ADDR_OR	   : requested address out of range
    `CMD_ERR_DATA_OR	   : requested data out of range
    `CMD_ERR_PERM_OR	   : request permission denied
    `CMD_ERR_WR_OR		   : write operation prohibited
    `CMD_ERR_UNKNOWN	   : unknown error
  */
static uint16_t cmd_wr_reg(void)
{
	uint8_t err_code;
	uint8_t tx_addr, tx_cnt;
	extern sys_reg_st g_sys;

	err_code = CMD_ERR_NOERR;
	tx_addr = cmd_reg_inst.rx_buf[FRAME_D_AL_POS] >> 16;
	tx_cnt = cmd_reg_inst.rx_buf[FRAME_D_AL_POS] & 0x0000ffff;

	cmd_reg_inst.rx_cnt = 0;								//clear rx_buffer
	cmd_reg_inst.rx_tag = 0;

	err_code = reg_map_write(tx_addr, &cmd_reg_inst.rx_buf[FRAME_D_PL_POS],	tx_cnt); 									//write conf reg map

	cmd_reg_inst.tx_buf[FRAME_D_AL_POS] = cmd_reg_inst.rx_buf[FRAME_D_AL_POS];

	cmd_reg_inst.tx_cnt = 1;
	cmd_reg_inst.tx_cmd = (cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] >> 16) & 0x00ff;
	cmd_reg_inst.tx_errcode = err_code;

	cmd_response();
	return err_code;
}

/**
  * @brief  cmd command write reg operation
	* @param  none
  * @retval 
		`CMD_ERR_NOERR			 : operation OK
		`CMD_ERR_ADDR_OR	   : requested address out of range
    `CMD_ERR_DATA_OR	   : requested data out of range
    `CMD_ERR_PERM_OR	   : request permission denied
    `CMD_ERR_WR_OR		   : write operation prohibited
    `CMD_ERR_UNKNOWN	   : unknown error
  */
static uint16_t cmd_rd_reg(void)
{
	uint8_t err_code;
	uint16_t rd_addr, rd_cnt;

	err_code = CMD_ERR_NOERR;

	rd_addr = cmd_reg_inst.rx_buf[FRAME_D_AL_POS] >> 16;
	rd_cnt = cmd_reg_inst.rx_buf[FRAME_D_AL_POS] & 0x0000ffff;

	cmd_reg_inst.rx_cnt = 0;								//clear rx_buffer
	cmd_reg_inst.rx_tag = 0;

	if (rd_cnt > CMD_RTX_BUF_DEPTH) {
		err_code = CMD_ERR_UNKNOWN;
		cmd_reg_inst.tx_cnt = 1;
	} else {
		err_code = reg_map_read(rd_addr, &cmd_reg_inst.tx_buf[FRAME_D_PL_POS],
				rd_cnt);
		cmd_reg_inst.tx_cnt = rd_cnt + 1;
	}
	cmd_reg_inst.tx_buf[FRAME_D_AL_POS] = cmd_reg_inst.rx_buf[FRAME_D_AL_POS];

	cmd_reg_inst.tx_cmd = (cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] >> 16) & 0x00ff;
	cmd_reg_inst.tx_errcode = err_code;
	cmd_response();
	return err_code;
}

/**
  * @brief  cmd command write reg operation
	* @param  none
  * @retval 
		`CMD_ERR_NOERR			 : operation OK
		`CMD_ERR_ADDR_OR	   : requested address out of range
    `CMD_ERR_DATA_OR	   : requested data out of range
    `CMD_ERR_PERM_OR	   : request permission denied
    `CMD_ERR_WR_OR		   : write operation prohibited
    `CMD_ERR_UNKNOWN	   : unknown error
  */
static uint16_t cmd_wr_ser(void)
{
	uint8_t err_code;
	uint8_t tx_cnt, i;
	uint8_t tx_buf[32];

	extern sys_reg_st g_sys;

	err_code = CMD_ERR_NOERR;

	tx_cnt = cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] & 0x0000ffff;

	cmd_reg_inst.rx_cnt = 0;								//clear rx_buffer
	cmd_reg_inst.rx_tag = 0;

	for (i = 0; i < tx_cnt; i++) {
		tx_buf[i] = (uint8_t) cmd_reg_inst.rx_buf[FRAME_D_AL_POS + i];
	}
//    err_code = plc_tx(tx_buf,tx_cnt);

	cmd_reg_inst.tx_cnt = 0;
	cmd_reg_inst.tx_cmd = (cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] >> 16) & 0x0fff;

	cmd_reg_inst.tx_errcode = err_code;

	cmd_response();
	return err_code;
}

/**
  * @brief  cmd command write reg operation
	* @param  none
  * @retval 
		`CMD_ERR_NOERR			 : operation OK
		`CMD_ERR_ADDR_OR	   : requested address out of range
    `CMD_ERR_DATA_OR	   : requested data out of range
    `CMD_ERR_PERM_OR	   : request permission denied
    `CMD_ERR_WR_OR		   : write operation prohibited
    `CMD_ERR_UNKNOWN	   : unknown error
  */
static uint16_t cmd_rd_ser(void)
{
//		uint8_t err_code;
//		uint8_t tx_cnt,i;
//    uint8_t tx_buf[32];
//    
//		extern sys_reg_st	 g_sys; 
//	
//    err_code = CMD_ERR_NOERR;					

//		tx_cnt = cmd_reg_inst.rx_buf[FRAME_C_ATL_POS]&0x0000ffff;

//		cmd_reg_inst.rx_cnt = 0;																																            //clear rx_buffer
//		cmd_reg_inst.rx_tag = 0;
//  
//    for(i=0;i<tx_cnt;i++)
//    {
//        tx_buf[i] = (uint8_t)cmd_reg_inst.rx_buf[FRAME_D_AL_POS+i];
//    }
//    err_code = plc_tx(tx_buf,tx_cnt);
//  
//    cmd_reg_inst.tx_cnt = 0;
//    cmd_reg_inst.tx_cmd	= (cmd_reg_inst.rx_buf[FRAME_C_ATL_POS]>>16)&0x0fff;
//    
//    cmd_reg_inst.tx_errcode = err_code;

//		cmd_response();
//		return err_code;
      return 0;
}


/**
  * @brief  cmd command write reg operation
	* @param  none
  * @retval 
		`CMD_ERR_NOERR			 : operation OK
		`CMD_ERR_ADDR_OR	   : requested address out of range
    `CMD_ERR_DATA_OR	   : requested data out of range
    `CMD_ERR_PERM_OR	   : request permission denied
    `CMD_ERR_WR_OR		   : write operation prohibited
    `CMD_ERR_UNKNOWN	   : unknown error
  */
uint16_t cmd_frame_resolve(void)
{
	uint8_t err_code;
	uint8_t frame_cmd_type;

	err_code = CMD_ERR_NOERR;
	frame_cmd_type = (cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] >> 16) & 0x00ff;

	if (cmd_reg_inst.rx_tag == 0) {
		err_code = CMD_ERR_NOERR;
		return err_code;
	}

	switch (frame_cmd_type) {
		case (CMD_RD_REG): {
			err_code = cmd_rd_reg();
			printf("console: read reg.\n");
			break;
		}
		case (CMD_WR_REG): {
			err_code = cmd_wr_reg();
			printf("console: write reg.\n");
			break;
		}
		case (CMD_RD_SER): {
			err_code = cmd_rd_ser();
			printf("console: read ser.\n");
			break;
		}
		case (CMD_WR_SER): {
			err_code = cmd_wr_ser();
			printf("console: write ser.\n");
			break;
		}
		default: {
			err_code = CMD_ERR_UNKNOWN;
			break;
		}
	}
	return err_code;
}

void recv_frame_fsm(void)
{
	uint32_t rx_data;

	if (cmd_reg_inst.rx_tag == 1)
		return;

	if (cmd_reg_inst.cmd_fsm_cstate == CMD_FRAME_FSM_DATA) {
		cmd_reg_inst.rtx_timeout++;
	}

	while ((fifo32_pop(&cmd_rx_fifo, &rx_data) == 1)
			&& (cmd_reg_inst.rx_tag == 0))
	{
//        printf("tcp: %x\n ",rx_data);
		switch (cmd_reg_inst.cmd_fsm_cstate)
		{
			case (CMD_FRAME_FSM_SYNC): {
				cmd_reg_inst.rx_cnt = 0;
				if (rx_data == CMD_FRAME_TAG_M_SYNC) {
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = rx_data;
					cmd_reg_inst.rx_cnt++;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_TL;
				} else {
					cmd_reg_inst.rx_cnt = 0;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;
				}
				cmd_reg_inst.rtx_timeout = 0;
				break;
			}
			case (CMD_FRAME_FSM_TL): {
				if (cmd_reg_inst.rtx_timeout < CMD_FSM_TIMEOUT) {
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = rx_data;
					cmd_reg_inst.rx_cnt++;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_DATA;
				} else {
					cmd_reg_inst.rx_cnt = 0;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;
				}
				cmd_reg_inst.rtx_timeout = 0;
	//                printf("fsm_len\n ");
				break;
			}
			case (CMD_FRAME_FSM_DATA): {
				if (cmd_reg_inst.rtx_timeout > CMD_FSM_TIMEOUT) {
					cmd_reg_inst.rx_cnt = 0;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
					cmd_reg_inst.rtx_timeout = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;
	//                    printf("cmd timeout\n");
				} else if (((cmd_reg_inst.rx_buf[FRAME_C_ATL_POS] & 0x0000ffff)
						+ CMD_FRAME_OVSIZE - 1) > cmd_reg_inst.rx_cnt) {
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = rx_data;
					cmd_reg_inst.rx_cnt++;
					cmd_reg_inst.rx_tag = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_DATA;
				} else {
					cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = rx_data;
					cmd_reg_inst.rx_cnt++;
					if (frame_checksum() == 1) {
						cmd_reg_inst.rx_tag = 1;
	//												cmd_reg_inst.rx_cnt = cmd_reg_inst.rx_cnt;
						cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
	//												printf("chk ok\n");
					} else {
						cmd_reg_inst.rx_tag = 0;
						cmd_reg_inst.rx_cnt = 0;
						cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
	//												printf("chk error\n");
					}
					cmd_reg_inst.rtx_timeout = 0;
					cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;
				}
	//                printf("fsm_data\n ");
				break;
			}
			default: {
				cmd_reg_inst.rx_cnt = 0;
				cmd_reg_inst.rx_tag = 0;
				cmd_reg_inst.rx_buf[cmd_reg_inst.rx_cnt] = 0;
				cmd_reg_inst.cmd_fsm_cstate = CMD_FRAME_FSM_SYNC;
				cmd_reg_inst.rtx_timeout = 0;
	//                printf("fsm_default\n ");
				break;
			}
		}
	}
}

//static int16_t get_report_data(uint32_t * buf_ptr)
//{
//    int16_t ret;
//    extern sys_reg_st	 g_sys;
//    extern mbm_dev_st mbm_dev_inst;
//    uint32_t hold_bitmap,input_bitmap,buf_temp;
//    uint16_t i,j,k;
//
//    ret = 0;
//    hold_bitmap = mbm_dev_inst.sts_bitmap_hold;
//    input_bitmap = mbm_dev_inst.sts_bitmap_input;
//
//    k = 0;
//    j = 0;
//    for(i=0;i<MBM_TOTAL_SLAVE_NUM;i++)
//    {
//        if(((hold_bitmap>>i)&0x0001) != 0)
//        {
//            *(buf_ptr+j) = (g_sys.conf.mbm.arr_slave_hbase_regn[i]&0x0000ffff)|(i<<16);
//            j++;
//            for(k=0;k<(g_sys.conf.mbm.arr_slave_hbase_regn[i]&0x0000ffff);k++)
//            {
//                buf_temp = mbm_dev_inst.holdbuf_reg[i][k];
//                *(buf_ptr+j+k) = buf_temp;
//            }
//            j = j+k;
//        }
//    }
//    for(i=0;i<MBM_TOTAL_SLAVE_NUM;i++)
//    {
//        if(((input_bitmap>>i)&0x0001) != 0)
//        {
//            *(buf_ptr+j) = (g_sys.conf.mbm.arr_slave_ibase_regn[i]&0x0000ffff)|(i<<16)|0x80000000;
//            j++;
//            for(k=0;k<(g_sys.conf.mbm.arr_slave_ibase_regn[i]&0x0000ffff);k++)
//            {
//                buf_temp = mbm_dev_inst.inputbuf_reg[i][k];
//                *(buf_ptr+j+k) = buf_temp;
//            }
//            j = j+k;
//        }
//    }
//    ret = j;
//    return ret;
//}

uint16_t report_data(void)
{
    extern sys_reg_st	  g_sys;
	uint8_t err_code;
	uint16_t rd_cnt;
	
    err_code = CMD_ERR_NOERR;
    if(bit_op_get(g_sys.stat.gen.status_bm,GBM_TCP) == 0)
        return CMD_NOT_READY;
  
//    rd_cnt = get_report_data(&cmd_reg_inst.tx_buf[FRAME_D_AL_POS]);
    rd_cnt = 0;
    
    if(rd_cnt == 0)
        return 0;
    
    cmd_reg_inst.tx_cmd	= CMD_RP_PKG;
    cmd_reg_inst.tx_cnt = rd_cnt;
    cmd_reg_inst.tx_errcode = err_code;
		cmd_response();
		return err_code;
}

static int16_t get_geo_data(uint32_t * buf_ptr)
{
    extern fifo32_cb_td geo_rx_fifo;
    uint32_t temp;
    uint16_t i;
    uint16_t fifo_len;
  
    fifo_len = get_fifo32_length(&geo_rx_fifo);
  
    for(i=0;i<fifo_len;i++)
    {
        fifo32_pop(&geo_rx_fifo,&temp);
        *(buf_ptr+i) = temp;
    }  
    return fifo_len;
}

uint16_t report_geo_data(void)
{
    extern sys_reg_st	  g_sys;
	uint8_t err_code;
	uint16_t rd_cnt;
	
    err_code = CMD_ERR_NOERR;
    if(bit_op_get(g_sys.stat.gen.status_bm,GBM_BT) == 0)
        return CMD_NOT_READY;
  
    rd_cnt = get_geo_data(&cmd_reg_inst.tx_buf[FRAME_D_AL_POS]);
    
    if(rd_cnt == 0)
        return 0;
    
    cmd_reg_inst.tx_cmd	= CMD_RP_GEO;
    cmd_reg_inst.tx_cnt = rd_cnt;
    cmd_reg_inst.tx_errcode = err_code;
	cmd_response();
	return err_code;
}



//const esp_timer_create_args_t periodic_timer_args = {
//        .callback = &periodic_timer_callback,
//        /* name is optional, but may help identify the timer when debugging */
//        .name = "periodic"
//};



//FINSH_FUNCTION_EXPORT(show_cmd_info, show cmd information.);
