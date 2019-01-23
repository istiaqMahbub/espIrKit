/* main.c -- MQTT client example
 *
 * Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Redis nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "mem.h"
#include "tcp_server.h"
#include "user_mqtt.h"
#include "esp_touch.h"
#include "user_uart.h"
#include "base64.h"

static MQTT_Client mqttClient;

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		os_printf("\r\nWIFI STAGot ip\r\n");
		ip_addr_t *addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));
		sntp_setservername(0, (char*)"nist.netservicesgroup.com");
		sntp_setservername(1,(char*) "time-c.nist.gov");
		IP4_ADDR(addr,210,72,145,44);
		sntp_setserver(2, addr); // set server 2 by IP address
		sntp_stop();
		sntp_set_timezone(6);
		sntp_init();
		os_free(addr);
		MQTT_Connect(&mqttClient);
	} else {
		os_printf("\r\nWIFI Disconnected\r\n");
		MQTT_Disconnect(&mqttClient);
	}
}


void user_rf_pre_init(void) {
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_set_softap_config
 * Description  : set SSID and password of ESP8266 softAP
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_set_softap_config(void) {
	struct softap_config config;

	struct ip_info info;
	wifi_set_opmode(STATIONAP_MODE); //Set softAP + station mode
	wifi_softap_dhcps_stop();
	IP4_ADDR(&info.ip, 192, 168, 5, 1);
	IP4_ADDR(&info.gw, 192, 168, 5, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	wifi_set_ip_info(SOFTAP_IF, &info);
	wifi_softap_dhcps_start();

	wifi_softap_get_config(&config); // Get config first.

	os_memset(config.ssid, 0, 32);
	os_memset(config.password, 0, 64);
	os_memcpy(config.ssid, sysCfg.meshCfg.mesh_ssid_prefix, os_strlen(sysCfg.meshCfg.mesh_ssid_prefix));
	os_memcpy(config.password, sysCfg.meshCfg.mesh_pass, os_strlen(sysCfg.meshCfg.mesh_pass));
	config.authmode = AUTH_WPA_WPA2_PSK;
	config.ssid_len = os_strlen(sysCfg.meshCfg.mesh_ssid_prefix); // or its actual length
	config.max_connection = 4; // how many stations can connect to ESP8266 softAP at most.
	wifi_softap_set_config(&config); // Set ESP8266 softap config .

}

void ICACHE_FLASH_ATTR
uartDataCallback(uint8_t cmd, uint8_t cmd_id, uint8_t *data, int len){
	os_printf("cmd: %d, cmd_id: %d data_len: %d\r\n", cmd,cmd_id,len);
	/*int i =0;
	for(i=0;i<len;i++){
		os_printf("data[%d]= %d \r\n",i,data[i]);
	}*/
	char buff[512];
	switch(cmd){
	case 2:
	case 4:
		os_bzero(buff, sizeof(buff));
		int encLen = base64_encode(buff, data, len);
		//os_printf("Encoded Buf, len %d, data =  %s\r\n",encLen,buff);
		char jsonBuf[1024];
		os_bzero(jsonBuf,sizeof(jsonBuf));
		user_ir_json_create(jsonBuf,cmd,cmd_id,encLen,buff);
		char cmdTopic[100];
		get_mqtt_ir_fb_topic(cmdTopic);
		MQTT_Publish(&mqttClient, cmdTopic, jsonBuf, os_strlen(jsonBuf), 2, 0);
		break;
	default:
		break;
	}
}


void ICACHE_FLASH_ATTR
irCmdCallback(uint8_t cmd, uint8_t cmd_id, uint8_t *data, int len){
	os_printf("cmd: %d, cmd_id: %d data_len: %d\r\n", cmd,cmd_id,len);
	int i =0;
	char buffer[800];
	switch(cmd){
	case 1:
		user_uart_packet_send(cmd, cmd_id, NULL, 0);
		break;
	case 3:
		/*for(i=0;i<len;i++){
			os_printf("data[%d]= %d \r\n",i,data[i]);
		}*/
		os_bzero(buffer,800);
		int decodedLen = base64_dec_len(data, len);
		base64_decode(buffer, data, len);
		user_uart_packet_send(cmd, cmd_id, buffer, decodedLen);
		break;
	default:
		break;
	}
}

void ICACHE_FLASH_ATTR
user_set_com_mode(void) {
	//struct station_config sta_conf;
	os_printf("Setting Meter Mode %d\r\n",sysCfg.deviceCfg.comMode);
		switch(sysCfg.deviceCfg.comMode){
			case MODE_MESH:
				knk_mesh_init();
			break;
			case MODE_SOFTAP:
				knk_mesh_deinit();
				user_set_softap_config();
			break;
			case MODE_STA:
				knk_mesh_deinit();
				wifi_station_disconnect();
				WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
			break;
		}
	user_uart_init(uartDataCallback);
	user_ir_cmd_init(irCmdCallback);
}

void userWebServerDataChangedCb(void){
	MQTT_Disconnect(&mqttClient);
	user_init_mqtt(&mqttClient);
}

void user_init(void) {
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);
	CFG_Load();
	INFO("\r\nSystem starting ...sdk v = %s\r\n", system_get_sdk_version());
	user_webserver_init(userWebServerDataChangedCb);
	user_init_mqtt(&mqttClient);
	user_uart_disable();
	esptouch_init();//Run this under GPIO interrupt handler
	//user_set_com_mode();
}



