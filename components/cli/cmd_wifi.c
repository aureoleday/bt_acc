/* Console example 鈥� WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "cmd_wifi.h"
#include "mqtt_client.h"
#include "web_sock.h"
#include "threads.h"
#include "sys_conf.h"
#include "bit_op.h"
#include "web_srv.h"

extern  sys_reg_st  g_sys;

static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int STARTED_BIT = BIT1;

//static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
//{
//    esp_mqtt_client_handle_t client = event->client;
//    int msg_id;
//    // your_context_t *context = event->context;
//    switch (event->event_id) {
//        case MQTT_EVENT_CONNECTED:
//            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
//            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
//
//            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
//            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
//
//            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
//            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
//
//            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
//            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
//            break;
//        case MQTT_EVENT_DISCONNECTED:
//            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//            break;
//
//        case MQTT_EVENT_SUBSCRIBED:
//            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
//            break;
//        case MQTT_EVENT_UNSUBSCRIBED:
//            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//            break;
//        case MQTT_EVENT_PUBLISHED:
//            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//            break;
//        case MQTT_EVENT_DATA:
//            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//            printf("DATA=%.*s\r\n", event->data_len, event->data);
//            break;
//        case MQTT_EVENT_ERROR:
//            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//            break;
//    }
//    return ESP_OK;
//}
//
//#define CONFIG_BROKER_URL  "mqtt://iot.eclipse.org"

//static void mqtt_app_start(void)
//{
//    esp_mqtt_client_config_t mqtt_cfg = {
//        .uri = CONFIG_BROKER_URL,
////   		.uri = "mqtt://192.168.56.1",
//        .event_handle = mqtt_event_handler,
//        // .user_context = (void *)your_context
//    };

//#if CONFIG_BROKER_URL_FROM_STDIN
//    char line[128];
//
//    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
//        int count = 0;
//        printf("Please enter url of mqtt broker\n");
//        while (count < 128) {
//            int c = fgetc(stdin);
//            if (c == '\n') {
//                line[count] = '\0';
//                break;
//            } else if (c > 0 && c < 127) {
//                line[count] = c;
//                ++count;
//            }
//            vTaskDelay(10 / portTICK_PERIOD_MS);
//        }
//        mqtt_cfg.uri = line;
//        printf("Broker url: %s\n", line);
//    } else {
//        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
//        abort();
//    }
//#endif /* CONFIG_BROKER_URL_FROM_STDIN */
//
//    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
//    esp_mqtt_client_start(client);
//}

static void connected_ops(void)
{
	xTaskCreate(&tcp_thread,
	    	     "Task_TCP",
	    	     8192,
	    	     NULL,
	    	     5,
	    	     NULL);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	httpd_handle_t *server = (httpd_handle_t *) ctx;

    switch(event->event_id) {
    case SYSTEM_EVENT_AP_START:
    	ESP_LOGI(TAG, "AP started!\n");
    	connected_ops();
    	break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        bit_op_set(&g_sys.stat.gen.status_bm,GBM_LINK,1);
        /* Start the web server */
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        bit_op_set(&g_sys.stat.gen.status_bm,GBM_LINK,0);
        /* Stop the web server */
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        bit_op_set(&g_sys.stat.gen.status_bm,GBM_LINK,1);
        /* Start the web server */
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        bit_op_set(&g_sys.stat.gen.status_bm,GBM_LINK,0);
        /* Stop the web server */
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void* arg)
{
	wifi_config_t wifi_config = {
		.ap = {
			.ssid = "geo_acc",
			.ssid_len = 7,
			.max_connection = 4,
			.password = "12345678",
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, arg) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

//    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    initialized = true;
}

static bool wifi_join(const char* ssid, const char* pass, int timeout_ms)
{
    wifi_config_t wifi_config = { 0 };
    strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strncpy((char*) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );

//    xTaskCreate(&task_process_ws, "ws_process_rx", 2048, NULL, 5, NULL);
//    //Create Websocket Server Task
//    xTaskCreate(&ws_server, "ws_server", 2048, NULL, 5, NULL);

    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
            1, 1, timeout_ms / portTICK_PERIOD_MS);

    return (bits & CONNECTED_BIT) != 0;
}


/** Arguments used by 'join' function */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} wifi_args;

static int connect(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Connecting to '%s'",
    		wifi_args.ssid->sval[0]);

    bool connected = wifi_join(wifi_args.ssid->sval[0],
    							wifi_args.password->sval[0],
								wifi_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    return 0;
}


void register_wifi()
{
    wifi_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    wifi_args.timeout->ival[0] = 5000; // set default value
    wifi_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    wifi_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    wifi_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &connect,
        .argtable = &wifi_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}

void usr_wifi_init(void)
{
	static httpd_handle_t server = NULL;
	initialise_wifi(&server);
}
