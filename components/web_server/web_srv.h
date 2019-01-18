/*
 * web_srv.h
 *
 *  Created on: 2019Äê1ÔÂ18ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_WEB_SERVER_WEB_SRV_H_
#define COMPONENTS_WEB_SERVER_WEB_SRV_H_

#include <esp_http_server.h>

void stop_webserver(httpd_handle_t server);
httpd_handle_t start_webserver(void);

#endif /* COMPONENTS_WEB_SERVER_WEB_SRV_H_ */
