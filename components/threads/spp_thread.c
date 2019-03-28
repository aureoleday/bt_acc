/*
 * spp_thread.c
 *
 *  Created on: 2019<C4><EA>1<D4><C2>4<C8><D5>
 *      Author: Administrator
 */


/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

#include "time.h"
#include "sys/time.h"

#include "sys_conf.h"
#include "bit_op.h"
#include "fifo.h"

#define SPP_TAG "SPP_GEO"
#define SPP_SERVER_NAME "GEO_YKJ"
#define EXCAMPLE_DEVICE_NAME "SPP_ACCEPTOR"
#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA    /*Choose show mode: show data or speed*/

typedef struct
{
       uint32_t h_conn[8];
       uint32_t conn_cnt;
}usr_spp_st;

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static struct timeval time_new, time_old;
static long data_num = 0;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static usr_spp_st usr_spp_inst;

static void usr_spp_init(void)
{
       int i;
       for(i=0;i<8;i++)
               usr_spp_inst.h_conn[i] = 0;
       usr_spp_inst.conn_cnt = 0;
}

static esp_err_t usr_spp_reg_conn(uint32_t h_conn,bool reg_mode)
{
       extern sys_reg_st  g_sys;
       esp_err_t ret = ESP_OK;

       if(reg_mode != 0)
       {
               if(usr_spp_inst.conn_cnt<8)
               {
                       usr_spp_inst.h_conn[usr_spp_inst.conn_cnt] = h_conn;
                       usr_spp_inst.conn_cnt++;
               }
               else
               {
                       ESP_LOGW("","spp conn full");
                       ret = ESP_FAIL;
               }
               bit_op_set(&g_sys.stat.gen.status_bm, GBM_BT, 1);
       }
       else
       {
               if(usr_spp_inst.conn_cnt <= 0)
               {
                       ESP_LOGW("","spp conn empty");
                       ret = ESP_FAIL;
               }
               else
               {
                       usr_spp_inst.h_conn[usr_spp_inst.conn_cnt] = 0;
                       usr_spp_inst.conn_cnt--;
               }
               bit_op_set(&g_sys.stat.gen.status_bm, GBM_BT, 0);
       }
       return ret;
}

esp_err_t usr_spp_write(uint8_t conn_id,uint8_t* src_dptr, uint16_t tx_len)
{
       return esp_spp_write(usr_spp_inst.h_conn[conn_id], tx_len, src_dptr);
}


static void print_speed(void)
{
    float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
    float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
    float time_interval = time_new_s - time_old_s;
    float speed = data_num * 8 / time_interval / 1000.0;
    ESP_LOGI(SPP_TAG, "speed(%fs ~ %fs): %f kbit/s" , time_old_s, time_new_s, speed);
    data_num = 0;
    time_old.tv_sec = time_new.tv_sec;
    time_old.tv_usec = time_new.tv_usec;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(EXCAMPLE_DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
        if (usr_spp_reg_conn(param->open.handle,0) != 0) {
               ESP_LOGW( SPP_TAG, "unregister SPP Connection failed");
        }
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                 param->data_ind.len, param->data_ind.handle);
        esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
#else
        gettimeofday(&time_new, NULL);
        data_num += param->data_ind.len;
        if (time_new.tv_sec - time_old.tv_sec >= 3) {
            print_speed();
        }
#endif
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
//        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
        if (usr_spp_reg_conn(param->open.handle,1) != 0) {
               ESP_LOGW( SPP_TAG, "register SPP Connection failed");
        }
        gettimeofday(&time_old, NULL);
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}

static void spp_init(void)
{
       esp_err_t ret=ESP_OK;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
}

static uint32_t tx_buf[1024];

void spp_thread(void* param)
{
//     extern fifo32_cb_td cmd_tx_fifo;
//     uint16_t buf_len;
//     uint16_t i;
//
//     spp_init();
//     usr_spp_init();
//
//     while(1)
//     {
//             if(usr_spp_inst.conn_cnt > 0)
//             {
//             if(is_fifo32_empty(&cmd_tx_fifo) == 0)
//             {
//                 buf_len = get_fifo32_length(&cmd_tx_fifo);
//                 for(i=0;i<buf_len;i++)
//                 {
//                     fifo32_pop(&cmd_tx_fifo, &tx_buf[i]);
//                 }
//                 usr_spp_write(0,(uint8_t *)tx_buf,buf_len*4);
////               printf("buf_len:%d\n",buf_len);
//             }
//             }
//             vTaskDelay(20 / portTICK_PERIOD_MS);
//     }
}

/** Arguments used by 'join' function */
static struct {
    struct arg_str *data;
    struct arg_end *end;
} usr_spp_args;

static int wr_spp(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &usr_spp_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, usr_spp_args.end, argv[0]);
        return 1;
    }

    usr_spp_write(0,usr_spp_args.data->sval[0],strlen(usr_spp_args.data->sval[0]));


       return 0;
}


static void register_wr_spp()
{
    usr_spp_args.data = arg_str1(NULL, NULL, "<c>", "write to spp");
    usr_spp_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {
        .command = "spp_wr",
        .help = "write spp",
        .hint = NULL,
        .func = &wr_spp,
        .argtable = &usr_spp_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


void usr_spp_register(void)
{
     register_wr_spp();
}

