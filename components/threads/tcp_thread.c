/*
 * tcp_thread.c
 *
 *  Created on: 2019年1月15日
 *      Author: Administrator
 */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "sys_conf.h"
#include "bit_op.h"
#include "fifo.h"

#define PORT 9996
#define BUFSZ 1024

static const char *TAG = "tTCP";

static uint32_t tx_buf[2048];
static uint8_t  rx_buf[BUFSZ];

extern  sys_reg_st  g_sys;
extern  fifo32_cb_td cmd_rx_fifo;
extern  fifo32_cb_td cmd_tx_fifo;

static uint8_t  tcp_flag;

static void tcp_tx_thread(void* parameter);
static void tcp_rx_thread(void* parameter);


void tcp_thread(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    xTaskHandle tx_xHandle,rx_xHandle;
    int sock;
    struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
    uint addrLen = sizeof(sourceAddr);

#ifdef CONFIG_EXAMPLE_IPV4
	struct sockaddr_in destAddr;
	destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(PORT);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;
	inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
	struct sockaddr_in6 destAddr;
	bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
	destAddr.sin6_family = AF_INET6;
	destAddr.sin6_port = htons(PORT);
	addr_family = AF_INET6;
	ip_protocol = IPPROTO_IPV6;
	inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

	int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
	if (listen_sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
	   // break;
	}
	ESP_LOGI(TAG, "Socket created");

	int opt = 1;
	 if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int))<0)
	 {
		 perror("setsockopt");
		 //exit(EXIT_FAILURE);
	 }

	int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
	}
	ESP_LOGI(TAG, "Socket binded");

	err = listen(listen_sock, 1);
	if (err != 0) {
		ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
	}
	ESP_LOGI(TAG, "Socket listening");


	while (1)
	{
		addrLen = sizeof(struct sockaddr_in6);
		sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
		if (sock < 0) {
			ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
			bit_op_set(&g_sys.stat.gen.status_bm,GBM_TCP,0);
			continue;
		}
		else
		{
			ESP_LOGI(TAG, "Socket accepted");
			bit_op_set(&g_sys.stat.gen.status_bm,GBM_TCP,1);

			xTaskCreate(&tcp_tx_thread,
						"thread_tcp_tx",
						2048,
						&sock,
						6,
						&tx_xHandle);

			xTaskCreate(&tcp_rx_thread,
						"thread_tcp_rx",
						2048,
						&sock,
						7,
						&rx_xHandle);
			while (1)
			{
				if((tcp_flag != 0)||(g_sys.conf.eth.tcp_en == 0)||(!bit_op_get(g_sys.stat.gen.status_bm,GBM_LINK)))
				{
					vTaskDelete(tx_xHandle);
					vTaskDelete(rx_xHandle);
					printf("detach tcp_rtx threads\n");
					vTaskDelay(10 / portTICK_PERIOD_MS);	//this line prevent system from crash
					tcp_flag = 0;
					break;
				}
				vTaskDelay(5 / portTICK_PERIOD_MS);
			}
//			shutdown(sock, 1);
			close(sock);
			bit_op_set(&g_sys.stat.gen.status_bm,GBM_TCP,0);
		}
	}
	vTaskDelete(NULL);
}


//			while (1)
//			{
//				int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
//				// Error occured during receiving
//				if (len < 0) {
//					ESP_LOGE(TAG, "recv failed: errno %d", errno);
//					break;
//				}
//				// Connection closed
//				else if (len == 0) {
//					ESP_LOGI(TAG, "Connection closed");
//					break;
//				}
//				// Data received
//				else
//				{
//					// Get the sender's ip address as string
//					if (sourceAddr.sin6_family == PF_INET)
//					{
//						inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
//					}
//					else if (sourceAddr.sin6_family == PF_INET6)
//					{
//						inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
//					}
//
//					rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
//					ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
//					ESP_LOGI(TAG, "%s", rx_buffer);
//
//					/*int err = send(sock, rx_buffer, len, 0);
//					if (err < 0) {
//						ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
//						break;
//					}*/
//				}
//			}
//
//		if (sock != -1)
//		{
//			ESP_LOGE(TAG, "Shutting down socket and restarting...");
//			shutdown(sock, 0);
//			close(sock);
//        }
//    }
//    vTaskDelete(NULL);
//}


static void tcp_tx_thread(void* parameter)
{
    int t_sock,i,ret;
    uint16_t buf_len;
	vTaskDelay(100 / portTICK_PERIOD_MS);

    t_sock = *(int*)parameter;

    fifo32_reset(&cmd_tx_fifo);

    ret = 0;
	while(1)
	{
		/* 发送数据到sock连接 */
		if(is_fifo32_empty(&cmd_tx_fifo) == 0)
		{
			buf_len = get_fifo32_length(&cmd_tx_fifo);
			for(i=0;i<buf_len;i++)
			{
				fifo32_pop(&cmd_tx_fifo, &tx_buf[i]);
			}
			ret = send(t_sock, tx_buf, (i<<2), 0);
			if (ret < 0)
			{
				printf("\nsend error, close socket %x.\r\n",ret);
				tcp_flag |= 2;
				vTaskDelay(1000000 / portTICK_PERIOD_MS);
			}
			else if (ret == 0)
			{
				/* 打印send函数返回值为0的警告信息 */
				printf("\nSend warning,send function return 0.\r\n");
			}
//			else
//			{
//				printf("%d.\n",ret);
//			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

static void tcp_rx_thread(void* parameter)
{
    int r_sock,i;
    int bytes_received;
    uint32_t host_data;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    r_sock = *(int*)parameter;

	while(1)
	{
		bytes_received = recv(r_sock, (char*)rx_buf, BUFSZ - 1, 0);
		if (bytes_received < 0)
		{
			printf("\nsock err %d.\r\n",bytes_received);
			  tcp_flag |= 1;
			  vTaskDelay(1000000 / portTICK_PERIOD_MS);
		}
		else if (bytes_received == 0)
		{
			printf("\nTcp connection close 0.\r\n");
			tcp_flag |= 1;
			vTaskDelay(1000000 / portTICK_PERIOD_MS);
		}
		else
		{
			for(i=0;i<(bytes_received>>2);i++)
			{
				host_data = ntohl(*(uint32_t *)(rx_buf+4*i));
				fifo32_push(&cmd_rx_fifo, &host_data);
			}
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}

