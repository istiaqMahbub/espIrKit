#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "c_types.h"

#define MESH_DEMO_PRINT  ets_printf
#define MESH_DEMO_STRLEN ets_strlen
#define MESH_DEMO_MEMCPY ets_memcpy
#define MESH_DEMO_MEMSET ets_memset
#define MESH_DEMO_FREE   os_free
#define MESH_DEMO_ZALLOC os_zalloc
#define MESH_DEMO_MALLOC os_malloc

#define MESH_GROUP_ID_LEN 6
static const uint16_t server_port = 7000;                  /*PORT USED BY USER IOT SERVER FOR MESH SERVICE*/
static const uint8_t server_ip[4] = {192, 168, 0, 105};   /*USER IOT SERVER IP ADDRESS*/
static const uint32_t UART_BAUD_RATIO = 19200;             /*UART BAUD RATIO*/
static const uint8_t MESH_GROUP_ID[MESH_GROUP_ID_LEN] = {0x18,0xfe,0x34,0x00,0x00,0x50};  /*MESH_GROUP_ID & MESH_SSID_PREFIX REPRESENTS ONE MESH NETWORK*/
static const uint8_t MESH_ROUTER_BSSID[6] = {0xF0, 0xB4, 0x29, 0x2C, 0x7C, 0x72}; /*MAC OF ROUTER*/

/*
 * please change MESH_ROUTER_SSID and MESH_ROUTER_PASSWD according to your router
 */
#define MESH_ROUTER_SSID     "kanokshome"      /*THE ROUTER SSID*/
#define MESH_ROUTER_PASSWD   "amijanina"    /*THE ROUTER PASSWORD*/
#define MESH_SSID_PREFIX     "MESH_DEMO"    /*SET THE DEFAULT MESH SSID PREFIX;THE FINAL SSID OF SOFTAP WOULD BE "MESH_SSID_PREFIX_X_YYYYYY"*/
#define MESH_AUTH            AUTH_WPA2_PSK  /*AUTH_MODE OF SOFTAP FOR EACH MESH NODE*/
#define MESH_PASSWD          "123123123"    /*SET PASSWORD OF SOFTAP FOR EACH MESH NODE*/
#define MESH_MAX_HOP         (4)            /*MAX_HOPS OF MESH NETWORK*/



#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

/*#ifndef ESP_MESH_SUPPORT
//#define ESP_MESH_SUPPORT 1
#endif*/




#define CFG_HOLDER	0x00FF55A7	/* Change this value to load default configurations */
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */
#define CLIENT_SSL_ENABLE



/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST			"m10.cloudmqtt.com" //or "mqtt.yourdomain.com"
#define MQTT_PORT			14081
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		120	 /*second*/

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			"qdqowxwy"
#define MQTT_PASS			"ZEp3mmEAYur-"

#define STA_SSID "testSSID"
#define STA_PASS "123456789"
#define STA_TYPE AUTH_WPA2_PSK


#define SOFTAP_SSID "knk_%08X"
#define SOFTAP_PASS "janina123"

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE		 		2048

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//#define PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/
#endif




#endif
