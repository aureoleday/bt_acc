/*
 * web_srv.c
 *
 *  Created on: 2019年1月18日
 *      Author: Administrator
 */

/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <string.h>
#include <esp_http_server.h>
#include "cJSON.h"
#include "global_var.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 * The examples use simple WiFi configuration that you can set via
 * 'make menuconfig'.
 * If you'd rather not, just change the below entries to strings
 * with the config you want -
 * ie. #define EXAMPLE_WIFI_SSID "mywifissid"
*/

static const char *TAG="HTTP";


static void js_err(char * err_info, char* cj_dst)
{
	cJSON * root =  cJSON_CreateObject();
	char *cj_src = NULL;

    cJSON_AddItemToObject(root, "status", cJSON_CreateString("fail"));//
    cJSON_AddItemToObject(root, "info", cJSON_CreateString(err_info));//

    cj_src = cJSON_PrintUnformatted(root);

    strcpy(cj_dst,cj_src);
    free(cj_src);
    cJSON_Delete(root);
}

static int cj_rd_reg(uint16_t reg_addr,uint16_t reg_cnt, char* cj_dst)
{
	cJSON * root =  cJSON_CreateObject();
	char *cj_src = NULL;
	uint32_t rd_buf[64];

    cJSON_AddItemToObject(root, "reg_addr", cJSON_CreateNumber(reg_addr));//根节点下添加
    cJSON_AddItemToObject(root, "reg_cnt", cJSON_CreateNumber(reg_cnt));

    reg_map_read(reg_addr,rd_buf,reg_cnt);

    cJSON_AddItemToObject(root, "reg_data", cJSON_CreateIntArray((int*)rd_buf,reg_cnt));

    cj_src = cJSON_PrintUnformatted(root);
    strcpy(cj_dst,cj_src);
    free(cj_src);
    cJSON_Delete(root);
    return strlen(cj_src);
}

static int cj_wr_reg(uint16_t reg_addr,uint32_t reg_data, char* cj_dst)
{
	cJSON * root =  cJSON_CreateObject();
	char *cj_src = NULL;
	uint16_t ret;

    cJSON_AddItemToObject(root, "reg_addr", cJSON_CreateNumber(reg_addr));//根节点下添加
    cJSON_AddItemToObject(root, "reg_data", cJSON_CreateNumber(reg_data));

    ret = reg_map_write(reg_addr,&reg_data,1);
    if(ret == 0)
    	cJSON_AddItemToObject(root, "status", cJSON_CreateString("success"));
    else
    	cJSON_AddItemToObject(root, "status", cJSON_CreateString("fail"));

    cj_src = cJSON_PrintUnformatted(root);
    strcpy(cj_dst,cj_src);
    free(cj_src);
    cJSON_Delete(root);
    return strlen(cj_src);
}

//static int cj_fft(uint32_t sample_cnt,char* cj_dst)
//{
//	cJSON * root =  cJSON_CreateObject();
//	char *cj_src = NULL;
//	uint16_t ret;
//
//    cJSON_AddItemToObject(root, "sample_rate", cJSON_CreateNumber(reg_data));
//    cJSON_AddItemToObject(root, "sample_cnt", cJSON_CreateNumber(sample_cnt));
//    cJSON_AddItemToObject(root, "sample_dat", cJSON_CreateNumber(reg_data));
//    cJSON_AddItemToObject(root, "status", cJSON_CreateNumber(reg_data));
//
//    ret = fft_get(sample_cnt);
//
//    if(ret == 0)
//    	cJSON_AddItemToObject(root, "reg_data", cJSON_CreateIntArray((int*)rd_buf,reg_cnt));
//    else
//    	cJSON_AddItemToObject(root, "status", cJSON_CreateString("fail"));
//
//    cj_src = cJSON_PrintUnformatted(root);
//    strcpy(cj_dst,cj_src);
//    free(cj_src);
//    cJSON_Delete(root);
//    return strlen(cj_src);
//}

esp_err_t rd_reg_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    uint16_t reg_addr,reg_cnt;
    char param[32];
    char * cj_buf = malloc(256);;
    reg_addr = 0;
    reg_cnt = 0;

    esp_err_t ret = 0;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "reg_addr", param, sizeof(param)) != ESP_OK)
            {
                ESP_LOGW(TAG, "Found URL query parameter => reg_addr=%d", atoi(param));
                ret = -1;
            }
            else
            {
            	reg_addr = atoi(param);
            }

            if (httpd_query_key_value(buf, "reg_cnt", param, sizeof(param)) != ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => reg_cnt=%d", atoi(param));
                ret = -1;
            }
            else
            {
            	reg_cnt = atoi(param);
            }
        }
        free(buf);
    }


    if(ret == 0)
    {
    	cj_rd_reg(reg_addr,reg_cnt,cj_buf);
    }
    else
    {
    	js_err("URL query mismatch!",cj_buf);
    }

    httpd_resp_send(req, cj_buf, strlen(cj_buf));
    free(cj_buf);

    return ret;
}

httpd_uri_t rd_reg = {
    .uri       = "/rd_reg",
    .method    = HTTP_GET,
    .handler   = rd_reg_get_handler,
    .user_ctx  = NULL
};

esp_err_t wr_reg_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    uint16_t reg_addr,reg_data;
    char param[32];

    char * cj_buf = malloc(256);

    reg_addr = 0;
    reg_data = 0;

    esp_err_t ret = 0;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "reg_addr", param, sizeof(param)) != ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => reg_addr=%d", atoi(param));
                ret = -1;
            }
            else
            	reg_addr = atoi(param);

            if (httpd_query_key_value(buf, "reg_data", param, sizeof(param)) != ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => reg_data=%d", atoi(param));
                ret = -1;
            }
            else
            	reg_data = atoi(param);
        }
        free(buf);
    }

    if(ret == 0)
    	cj_wr_reg(reg_addr,reg_data,cj_buf);
    else
    	js_err("URL query mismatch!",cj_buf);

    httpd_resp_send(req, cj_buf, strlen(cj_buf));
    free(cj_buf);

    return ESP_OK;
}

httpd_uri_t wr_reg = {
    .uri       = "/wr_reg",
    .method    = HTTP_GET,
    .handler   = wr_reg_get_handler,
    .user_ctx  = NULL
};

esp_err_t fft_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    uint16_t reg_addr,reg_data;
    char param[32];

    char * cj_buf = malloc(256);

    reg_addr = 0;
    reg_data = 0;

    esp_err_t ret = 0;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "reg_addr", param, sizeof(param)) != ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => reg_addr=%d", atoi(param));
                ret = -1;
            }
            else
            	reg_addr = atoi(param);
        }
        free(buf);
    }

    if(ret == 0)
    	cj_wr_reg(reg_addr,reg_data,cj_buf);
    else
    	js_err("URL query mismatch!",cj_buf);

    httpd_resp_send(req, cj_buf, strlen(cj_buf));
    free(cj_buf);

    return ESP_OK;
}

httpd_uri_t fft = {
    .uri       = "/fft",
    .method    = HTTP_GET,
    .handler   = fft_get_handler,
    .user_ctx  = NULL
};

/* An HTTP GET handler */
esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    uint16_t reg_addr,reg_cnt;

    char * cj_buf = malloc(64);

    reg_addr = 0;
    reg_cnt = 0;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "reg_addr", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => reg_addr=%d", atoi(param));
                reg_addr = atoi(param);
            }
            if (httpd_query_key_value(buf, "reg_cnt", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => reg_cnt=%d", atoi(param));
                reg_cnt = atoi(param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
//    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
//    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
//    const char* resp_str = (const char*) req->user_ctx;
//    httpd_resp_send(req, resp_str, strlen(resp_str));
    cj_rd_reg(reg_addr,reg_cnt,cj_buf);
    httpd_resp_send(req, cj_buf, strlen(cj_buf));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    free(cj_buf);

    return ESP_OK;
}

httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

/* An HTTP POST handler */
esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* Handler can be unregistered using the uri string */
        ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
    }
    else {
        ESP_LOGI(TAG, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
        httpd_register_uri_handler(req->handle, &echo);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        httpd_register_uri_handler(server, &rd_reg);
        httpd_register_uri_handler(server, &wr_reg);
        httpd_register_uri_handler(server, &fft);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}
