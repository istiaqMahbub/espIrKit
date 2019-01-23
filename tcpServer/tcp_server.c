/*
 * tcp_server.c
 *
 *  Created on: Oct 2, 2016
 *      Author: kanok
 */
#include "ets_sys.h"
#include "osapi.h"
//#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

#include "master_mesh_parser.h"

#include "tcp_server.h"

LOCAL struct espconn esp_conn;
LOCAL esp_tcp esptcp;

/******************************************************************************
 * FunctionName : tcp_server_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_sent_cb(void *arg) {
	//data sent successfully

	os_printf("tcp sent cb \r\n");
	struct espconn *pespconn = arg;
	os_printf("remote ip: %d.%d.%d.%d \r\n", pespconn->proto.tcp->remote_ip[0],
			pespconn->proto.tcp->remote_ip[1],
			pespconn->proto.tcp->remote_ip[2],
			pespconn->proto.tcp->remote_ip[3]);
	os_printf("remote port: %d \r\n", pespconn->proto.tcp->remote_port);
}

/******************************************************************************
 * FunctionName : tcp_server_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_recv_cb(void *arg, char *pusrdata, unsigned short length) {
	//received some data from tcp connection
	struct espconn *pespconn = arg;
	/*os_printf("remote ip: %d.%d.%d.%d \r\n", pespconn->proto.tcp->remote_ip[0],
	 pespconn->proto.tcp->remote_ip[1], pespconn->proto.tcp->remote_ip[2],
	 pespconn->proto.tcp->remote_ip[3]);
	 os_printf("remote port: %d \r\n", pespconn->proto.tcp->remote_port);*/

	if (!pusrdata)
		return;
	mesh_packet_parser(arg, pusrdata, length);
	//espconn_sent(pespconn, pusrdata, length);
}

/******************************************************************************
 * FunctionName : tcp_server_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_discon_cb(void *arg) {
	//tcp disconnect successfully

	os_printf("tcp disconnect succeed !!! \r\n");
}

/******************************************************************************
 * FunctionName : tcp_server_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_recon_cb(void *arg, sint8 err) {
	//error occured , tcp connection broke.

	os_printf("reconnect callback, error code %d !!! \r\n", err);
}

LOCAL void tcp_server_multi_send(void) {
	struct espconn *pesp_conn = &esp_conn;

	remote_info *premot = NULL;
	uint8 count = 0;
	sint8 value = ESPCONN_OK;
	if (espconn_get_connection_info(pesp_conn, &premot, 0) == ESPCONN_OK) {
		char *pbuf = "tcp_server_multi_send\n";
		for (count = 0; count < pesp_conn->link_cnt; count++) {
			pesp_conn->proto.tcp->remote_port = premot[count].remote_port;

			pesp_conn->proto.tcp->remote_ip[0] = premot[count].remote_ip[0];
			pesp_conn->proto.tcp->remote_ip[1] = premot[count].remote_ip[1];
			pesp_conn->proto.tcp->remote_ip[2] = premot[count].remote_ip[2];
			pesp_conn->proto.tcp->remote_ip[3] = premot[count].remote_ip[3];

			//espconn_sent(pesp_conn, pbuf, os_strlen(pbuf));
		}
	}
}

bool ICACHE_FLASH_ATTR
mesh_send_packet(uint8_t *mesh_packet, uint8_t mesh_packet_len) {
	struct espconn *pesp_conn = &esp_conn;

	remote_info *premot = NULL;
	uint8 count = 0;
	sint8 value = ESPCONN_OK;
	if (espconn_get_connection_info(pesp_conn, &premot, 0) == ESPCONN_OK) {
		char *pbuf = "tcp_server_multi_send\n";
		for (count = 0; count < pesp_conn->link_cnt; count++) {
			pesp_conn->proto.tcp->remote_port = premot[count].remote_port;

			pesp_conn->proto.tcp->remote_ip[0] = premot[count].remote_ip[0];
			pesp_conn->proto.tcp->remote_ip[1] = premot[count].remote_ip[1];
			pesp_conn->proto.tcp->remote_ip[2] = premot[count].remote_ip[2];
			pesp_conn->proto.tcp->remote_ip[3] = premot[count].remote_ip[3];
			value = espconn_send(pesp_conn, mesh_packet, mesh_packet_len);
			os_printf("\r\nespconn  err: %d\r\n", value);
		}
	}
	return false;
}

/******************************************************************************
 * FunctionName : tcp_server_listen
 * Description  : TCP server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_listen(void *arg) {
	struct espconn *pesp_conn = arg;
	os_printf("tcp_server_listen !!! \r\n");

	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
	espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
	espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);

	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
	tcp_server_multi_send();
}

/******************************************************************************
 * FunctionName : user_tcpserver_init
 * Description  : parameter initialize as a TCP server
 * Parameters   : port -- server port
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_tcpserver_init(uint32 port) {
	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, tcp_server_listen);

	sint8 ret = espconn_accept(&esp_conn);

	os_printf("espconn_accept [%d] !!! \r\n", ret);

}
LOCAL os_timer_t test_timer;

/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(void) {
	struct ip_info ipconfig;

	//disarm timer first
	os_timer_disarm(&test_timer);

	//get ip info of ESP8266 station
	wifi_get_ip_info(SOFTAP_IF, &ipconfig);

	if (ipconfig.ip.addr != 0) {

		os_printf("got ip !!! \r\n");
		user_tcpserver_init(SERVER_LOCAL_PORT);

	} else {
		//re-arm timer to check ip
		os_timer_setfn(&test_timer,
				(os_timer_func_t *) user_esp_platform_check_ip, NULL);
		os_timer_arm(&test_timer, 100, 0);
	}
}

void ICACHE_FLASH_ATTR
init_tcp_server(void) {

	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer, (os_timer_func_t *) user_esp_platform_check_ip,
	NULL);
	os_timer_arm(&test_timer, 100, 0);
}
