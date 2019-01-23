/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "user_json.h"
#include "user_esp_platform.h"
#include "user_webserver.h"
#include "config.h"
#ifdef SERVER_SSL_ENABLE
#include "ssl/cert.h"
#include "ssl/private_key.h"
#endif
LOCAL struct station_config *sta_conf;
LOCAL struct softap_config *ap_conf;
LOCAL ip_addr_t local_ip;

LOCAL os_timer_t test_timer;

user_webserverCallback onDataChangedCbFunc = NULL;

LOCAL int ICACHE_FLASH_ATTR
connect_status_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "status", 8) == 0) {
		jsontree_write_int(js_ctx, user_esp_platform_get_connect_status());
	}

	return 0;
}
LOCAL struct jsontree_callback connect_status_callback =
		JSONTREE_CALLBACK(connect_status_get, NULL);

JSONTREE_OBJECT(status_sub_tree,
		JSONTREE_PAIR("status", &connect_status_callback));

JSONTREE_OBJECT(connect_status_tree, JSONTREE_PAIR("Status", &status_sub_tree));

JSONTREE_OBJECT(con_status_tree, JSONTREE_PAIR("info", &connect_status_tree));



/******************************************************************************
 * FunctionName : wifi_station_get
 * Description  : set up the station paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
mesh_config_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	uint8 buf[20];
	os_bzero(buf, sizeof(buf));

	if (os_strncmp(path, "MeshId", 6) == 0) {
		os_sprintf(buf, ID6STR, ID6TOSTR(&sysCfg.meshCfg.mesh_group_id));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "MeshSSID", 8) == 0) {
		jsontree_write_string(js_ctx, sysCfg.meshCfg.mesh_ssid_prefix);
	} else if (os_strncmp(path, "MeshPasswd", 10) == 0) {
		jsontree_write_string(js_ctx, sysCfg.meshCfg.mesh_pass);
	} else if (os_strncmp(path, "isMeshEnable", 12) == 0) {
		jsontree_write_string(js_ctx, sysCfg.meshCfg.isMeshEnable?"1":"0");
	} else if (os_strncmp(path, "server_host", 11) == 0) {
		os_sprintf(buf, "%s", sysCfg.serverCfg.server_host);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "server_port", 11) == 0) {
		os_sprintf(buf,"%d",sysCfg.serverCfg.server_port);
		jsontree_write_string(js_ctx,buf);
	}

	return 0;
}

/******************************************************************************
 * FunctionName : wifi_station_set
 * Description  : parse the station parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
mesh_config_set(struct jsontree_context *js_ctx,
		struct jsonparse_state *parser) {
	int type;
	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, 64);

			if (jsonparse_strcmp_value(parser, "MeshId") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				if(os_strlen(buffer) == MESH_GROUP_ID_LEN*3-1){
					os_printf("MeshId: %s\r\n",buffer);
					str_mac_to_byte_mac(buffer, sysCfg.meshCfg.mesh_group_id);
				} else{
					os_printf("Invalid MeshID\r\n");
				}
			} else if (jsonparse_strcmp_value(parser, "MeshSSID") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("MeshSSID: %s\r\n",buffer);
				os_bzero(sysCfg.meshCfg.mesh_ssid_prefix, sizeof(sysCfg.meshCfg.mesh_ssid_prefix));
				os_sprintf(sysCfg.meshCfg.mesh_ssid_prefix, "%s_%08X", buffer,system_get_chip_id());
			} else if (jsonparse_strcmp_value(parser, "MeshPasswd") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("MeshPasswd: %s\r\n",buffer);
				os_bzero(sysCfg.meshCfg.mesh_pass, sizeof(sysCfg.meshCfg.mesh_pass));
				os_sprintf(sysCfg.meshCfg.mesh_pass, "%s", buffer);
			} else if (jsonparse_strcmp_value(parser, "isMeshEnable") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("isMeshEnable: %s\r\n",buffer);
				if(os_strncmp(buffer,"1",1) == 0){
					sysCfg.meshCfg.isMeshEnable = 1;
				} else{
					sysCfg.meshCfg.isMeshEnable = 0;
				}
			} else if (jsonparse_strcmp_value(parser, "server_host") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("server_host: %s\r\n",buffer);
				os_bzero(sysCfg.serverCfg.server_host, sizeof(sysCfg.serverCfg.server_host));
				os_sprintf(sysCfg.serverCfg.server_host, "%s", buffer);
			} else if (jsonparse_strcmp_value(parser, "server_port") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("server_port: %s\r\n",buffer);
				sysCfg.serverCfg.server_port = Atoi(buffer);
			}
		}
	}
	CFG_Save();
	if(sysCfg.meshCfg.isMeshEnable == 1){
		user_set_com_mode();
	}
	os_printf("done mesh parsing\r\n");
	return 0;
}
LOCAL struct jsontree_callback mesh_config_callback =
		JSONTREE_CALLBACK(mesh_config_get, mesh_config_set);

JSONTREE_OBJECT(
		get_mesh_config_tree,
		JSONTREE_PAIR("MeshId", &mesh_config_callback), JSONTREE_PAIR("MeshSSID", &mesh_config_callback), JSONTREE_PAIR("MeshPasswd", &mesh_config_callback), JSONTREE_PAIR("isMeshEnable", &mesh_config_callback));

JSONTREE_OBJECT(
		set_mesh_config_tree,
		JSONTREE_PAIR("MeshId", &mesh_config_callback), JSONTREE_PAIR("MeshSSID", &mesh_config_callback), JSONTREE_PAIR("MeshPasswd", &mesh_config_callback), JSONTREE_PAIR("isMeshEnable", &mesh_config_callback));

JSONTREE_OBJECT(
		server_ip_tree,
		JSONTREE_PAIR("server_host", &mesh_config_callback), JSONTREE_PAIR("server_port", &mesh_config_callback));

JSONTREE_OBJECT(
		get_mesh_info_tree,
		JSONTREE_PAIR("MeshConfig", &get_mesh_config_tree), JSONTREE_PAIR("MeshserverIP", &server_ip_tree));
JSONTREE_OBJECT(set_mesh_info_tree,
		JSONTREE_PAIR("MeshConfig", &set_mesh_config_tree), JSONTREE_PAIR("MeshserverIP", &server_ip_tree));

JSONTREE_OBJECT(
		mesh_config_resp,
		JSONTREE_PAIR("MeshResp", &get_mesh_info_tree));
JSONTREE_OBJECT(mesh_config_req,
		JSONTREE_PAIR("MeshReq", &set_mesh_info_tree));



/******************************************************************************
 * FunctionName : wifi_station_get
 * Description  : set up the station paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
device_cfg_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	uint8 buf[20];
	os_bzero(buf, sizeof(buf));
	if (os_strncmp(path, "local_ip", 8) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&local_ip));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "strId", 5) == 0) {
		os_sprintf(buf, "%s", sysCfg.device_id);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "shMac", 5) == 0) {
		os_sprintf(buf, "%s", sysCfg.deviceCfg.shMac);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "com_mode_device", 15) == 0) {
		os_sprintf(buf, "%d", sysCfg.deviceCfg.comMode);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "type_device", 11) == 0) {
		os_sprintf(buf, "%s", sysCfg.deviceCfg.deviceType);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "name_device", 11) == 0) {
		os_sprintf(buf, "%s", sysCfg.deviceCfg.deviceName);
		jsontree_write_string(js_ctx, buf);
	}

	return 0;
}

/******************************************************************************
 * FunctionName : wifi_station_set
 * Description  : parse the station parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
device_cfg_set(struct jsontree_context *js_ctx,
		struct jsonparse_state *parser) {
	int type;
	bool isMacChanged = false;
	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, 64);
			if (jsonparse_strcmp_value(parser, "type_device") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("device_type: %s\r\n",buffer);
				if(os_strlen(buffer)<sizeof(sysCfg.deviceCfg.deviceType)){
					os_bzero(sysCfg.deviceCfg.deviceType, sizeof(sysCfg.deviceCfg.deviceType));
					os_sprintf(sysCfg.deviceCfg.deviceType, "%s", buffer);
				}else{
					os_printf("device_type len too big\r\n");
				}
			} else if (jsonparse_strcmp_value(parser, "name_device") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("device_name: %s\r\n",buffer);
				if(os_strlen(buffer)<sizeof(sysCfg.deviceCfg.deviceName)){
					os_bzero(sysCfg.deviceCfg.deviceName, sizeof(sysCfg.deviceCfg.deviceName));
					os_sprintf(sysCfg.deviceCfg.deviceName, "%s", buffer);
				}else
				{
					os_printf("device_name len too big\r\n");
				}
			} else if (jsonparse_strcmp_value(parser, "shMac") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("shMac: %s\r\n",buffer);
				if(os_strlen(buffer)<sizeof(sysCfg.deviceCfg.shMac)){
					os_bzero(sysCfg.deviceCfg.shMac, sizeof(sysCfg.deviceCfg.shMac));
					os_sprintf(sysCfg.deviceCfg.shMac, "%s", buffer);
					isMacChanged = true;
				}else
				{
					os_printf("sh_mac len too big\r\n");
				}
			}
		}
	}
	os_printf("done device parsing\r\n");
	CFG_Save();
	if(isMacChanged){
		if(onDataChangedCbFunc!=NULL){
			onDataChangedCbFunc();
		}
	}
	return 0;
}
LOCAL struct jsontree_callback device_cfg_callback =
		JSONTREE_CALLBACK(device_cfg_get, device_cfg_set);

JSONTREE_OBJECT(
		get_device_cfg_tree,
		JSONTREE_PAIR("local_ip", &device_cfg_callback),\
		JSONTREE_PAIR("strId", &device_cfg_callback), \
		JSONTREE_PAIR("com_mode_device", &device_cfg_callback),\
		JSONTREE_PAIR("shMac", &device_cfg_callback), \
		JSONTREE_PAIR("type_device", &device_cfg_callback), \
		JSONTREE_PAIR("name_device", &device_cfg_callback), \
		);

JSONTREE_OBJECT(
		set_device_cfg_tree,
		JSONTREE_PAIR("shMac", &device_cfg_callback), \
		JSONTREE_PAIR("type_device", &device_cfg_callback), \
		JSONTREE_PAIR("name_device", &device_cfg_callback), \
		);

JSONTREE_OBJECT(
		get_device_tree,
		JSONTREE_PAIR("DeviceCfgInfo", &get_device_cfg_tree));
JSONTREE_OBJECT(set_device_tree,
		JSONTREE_PAIR("DeviceCfgInfo", &set_device_cfg_tree));

JSONTREE_OBJECT(
		device_cfg_resp,
		JSONTREE_PAIR("DeviceCfgResp", &get_device_tree));
JSONTREE_OBJECT(device_cfg_req,
		JSONTREE_PAIR("DeviceCfgReq", &set_device_tree));





//step 4: define callback function for get/set

/******************************************************************************
 * FunctionName : wifi_station_get
 * Description  : set up the station paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
mqtt_info_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	uint8 buf[64];
	os_bzero(buf, sizeof(buf));
	if (os_strncmp(path, "local_ip", 8) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&local_ip));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "strId", 5) == 0) {
		os_sprintf(buf, "%s", sysCfg.device_id);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "dev_com_mode", 12) == 0) {
		os_sprintf(buf, "%d", sysCfg.deviceCfg.comMode);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "host_mqtt", 9) == 0) {
		os_sprintf(buf, "%s", sysCfg.mqtt_host);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "port_mqtt", 9) == 0) {
		os_sprintf(buf, "%ld", sysCfg.mqtt_port);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "user_mqtt", 9) == 0) {
		os_sprintf(buf, "%s", sysCfg.mqtt_user);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "pass_mqtt", 9) == 0) {
		os_sprintf(buf, "%s", sysCfg.mqtt_pass);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "ssl_mqtt", 8) == 0) {
		os_sprintf(buf, "%d", sysCfg.security);
		jsontree_write_string(js_ctx, buf);
	}
	return 0;
}

/******************************************************************************
 * FunctionName : wifi_station_set
 * Description  : parse the station parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
mqtt_info_set(struct jsontree_context *js_ctx,
		struct jsonparse_state *parser) {
	int type;
	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, 64);
			if (jsonparse_strcmp_value(parser, "host_mqtt") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("mqtt_host: %s\r\n",buffer);
				os_bzero(sysCfg.mqtt_host, sizeof(sysCfg.mqtt_host));
				os_sprintf(sysCfg.mqtt_host, "%s", buffer);
			} else if (jsonparse_strcmp_value(parser, "port_mqtt") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("mqtt_port: %s\r\n",buffer);
				sysCfg.mqtt_port = Atoi(buffer);
			} else if (jsonparse_strcmp_value(parser, "user_mqtt") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("mqtt_user: %s\r\n",buffer);
				os_bzero(sysCfg.mqtt_user, sizeof(sysCfg.mqtt_user));
				os_sprintf(sysCfg.mqtt_user, "%s", buffer);
			} else if (jsonparse_strcmp_value(parser, "pass_mqtt") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("mqtt_pass: %s\r\n",buffer);
				os_bzero(sysCfg.mqtt_pass, sizeof(sysCfg.mqtt_pass));
				os_sprintf(sysCfg.mqtt_pass, "%s", buffer);
			} else if (jsonparse_strcmp_value(parser, "ssl_mqtt") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("mqtt_ssl: %s\r\n",buffer);
				sysCfg.security = Atoi(buffer);
			}
		}
	}
	os_printf("done mqtt set parsing\r\n");
	CFG_Save();
	if(onDataChangedCbFunc!=NULL){
		onDataChangedCbFunc();
	}
	return 0;
}





//step 3: declare callback function for get/set
LOCAL struct jsontree_callback mqtt_info_callback =
		JSONTREE_CALLBACK(mqtt_info_get, mqtt_info_set);
//step 3: end


//step 2: define get set tree
JSONTREE_OBJECT(
		get_mqtt_info_tree,
		JSONTREE_PAIR("local_ip", &mqtt_info_callback),\
		JSONTREE_PAIR("strId", &mqtt_info_callback),\
		JSONTREE_PAIR("dev_com_mode", &mqtt_info_callback),\
		JSONTREE_PAIR("host_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("port_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("user_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("pass_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("ssl_mqtt", &mqtt_info_callback),\
		);

JSONTREE_OBJECT(
		set_mqtt_info_tree,
		JSONTREE_PAIR("host_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("port_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("user_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("pass_mqtt", &mqtt_info_callback),\
		JSONTREE_PAIR("ssl_mqtt", &mqtt_info_callback),\
		);
///////step 2: end

//step 1: define root tree
JSONTREE_OBJECT(
		mqtt_info_resp,
		JSONTREE_PAIR("MqttInfoResp", &get_mqtt_info_tree));

JSONTREE_OBJECT(
		mqtt_info_req,
		JSONTREE_PAIR("MqttInfoReq", &set_mqtt_info_tree));
//step 1: end












/******************************************************************************
 * FunctionName : wifi_station_get
 * Description  : set up the station paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
wifi_station_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	struct ip_info ipconfig;
	uint8 buf[20];
	os_bzero(buf, sizeof(buf));
	wifi_station_get_config(sta_conf);
	wifi_get_ip_info(STATION_IF, &ipconfig);

	if (os_strncmp(path, "ssid", 4) == 0) {
		jsontree_write_string(js_ctx, sta_conf->ssid);
	} else if (os_strncmp(path, "password", 8) == 0) {
		jsontree_write_string(js_ctx, sta_conf->password);
	} else if (os_strncmp(path, "ip", 2) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.ip));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "mask", 4) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.netmask));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "gw", 2) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.gw));
		jsontree_write_string(js_ctx, buf);
	}

	return 0;
}

/******************************************************************************
 * FunctionName : wifi_station_set
 * Description  : parse the station parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
wifi_station_set(struct jsontree_context *js_ctx,
		struct jsonparse_state *parser) {
	int type;
	uint8 station_tree;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, 64);

			if (jsonparse_strcmp_value(parser, "Station") == 0) {
				station_tree = 1;
			} else if (jsonparse_strcmp_value(parser, "Softap") == 0) {
				station_tree = 0;
			}

			if (station_tree) {
				if (jsonparse_strcmp_value(parser, "ssid") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					jsonparse_copy_value(parser, buffer, sizeof(buffer));
					os_memcpy(sta_conf->ssid, buffer, os_strlen(buffer));
				} else if (jsonparse_strcmp_value(parser, "password") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					jsonparse_copy_value(parser, buffer, sizeof(buffer));
					os_memcpy(sta_conf->password, buffer, os_strlen(buffer));
				}
			}
		}
	}

	return 0;
}
LOCAL struct jsontree_callback wifi_station_callback =
		JSONTREE_CALLBACK(wifi_station_get, wifi_station_set);

JSONTREE_OBJECT(
		get_station_config_tree,
		JSONTREE_PAIR("ssid", &wifi_station_callback), JSONTREE_PAIR("password", &wifi_station_callback));
JSONTREE_OBJECT(
		set_station_config_tree,
		JSONTREE_PAIR("ssid", &wifi_station_callback), JSONTREE_PAIR("password", &wifi_station_callback), JSONTREE_PAIR("token", &wifi_station_callback));

JSONTREE_OBJECT(
		ip_tree,
		JSONTREE_PAIR("ip", &wifi_station_callback), JSONTREE_PAIR("mask", &wifi_station_callback), JSONTREE_PAIR("gw", &wifi_station_callback));
JSONTREE_OBJECT(
		get_station_tree,
		JSONTREE_PAIR("Connect_Station", &get_station_config_tree), JSONTREE_PAIR("Ipinfo_Station", &ip_tree));
JSONTREE_OBJECT(set_station_tree,
		JSONTREE_PAIR("Connect_Station", &set_station_config_tree));

/******************************************************************************
 * FunctionName : wifi_softap_get
 * Description  : set up the softap paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
wifi_softap_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	struct ip_info ipconfig;
	uint8 buf[20];
	os_bzero(buf, sizeof(buf));
	wifi_softap_get_config(ap_conf);
	wifi_get_ip_info(SOFTAP_IF, &ipconfig);

	if (os_strncmp(path, "ssid", 4) == 0) {
		jsontree_write_string(js_ctx, ap_conf->ssid);
	} else if (os_strncmp(path, "password", 8) == 0) {
		jsontree_write_string(js_ctx, ap_conf->password);
	} else if (os_strncmp(path, "channel", 7) == 0) {
		jsontree_write_int(js_ctx, ap_conf->channel);
	} else if (os_strncmp(path, "authmode", 8) == 0) {
		switch (ap_conf->authmode) {
		case AUTH_OPEN:
			jsontree_write_string(js_ctx, "OPEN");
			break;

		case AUTH_WEP:
			jsontree_write_string(js_ctx, "WEP");
			break;

		case AUTH_WPA_PSK:
			jsontree_write_string(js_ctx, "WPAPSK");
			break;

		case AUTH_WPA2_PSK:
			jsontree_write_string(js_ctx, "WPA2PSK");
			break;

		case AUTH_WPA_WPA2_PSK:
			jsontree_write_string(js_ctx, "WPAPSK/WPA2PSK");
			break;

		default:
			jsontree_write_int(js_ctx, ap_conf->authmode);
			break;
		}
	} else if (os_strncmp(path, "ip", 2) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.ip));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "mask", 4) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.netmask));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "gw", 2) == 0) {
		os_sprintf(buf, IPSTR, IP2STR(&ipconfig.gw));
		jsontree_write_string(js_ctx, buf);
	}

	return 0;
}

/******************************************************************************
 * FunctionName : wifi_softap_set
 * Description  : parse the softap parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
wifi_softap_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser) {
	int type;
	uint8 softap_tree;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, 64);

			if (jsonparse_strcmp_value(parser, "Station") == 0) {
				softap_tree = 0;
			} else if (jsonparse_strcmp_value(parser, "Softap") == 0) {
				softap_tree = 1;
			}

			if (softap_tree) {
				if (jsonparse_strcmp_value(parser, "authmode") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					jsonparse_copy_value(parser, buffer, sizeof(buffer));

					// other mode will be supported later...
					if (os_strcmp(buffer, "OPEN") == 0) {
						ap_conf->authmode = AUTH_OPEN;
					} else if (os_strcmp(buffer, "WPAPSK") == 0) {
						ap_conf->authmode = AUTH_WPA_PSK;
						os_printf("%d %s\n", ap_conf->authmode, buffer);
					} else if (os_strcmp(buffer, "WPA2PSK") == 0) {
						ap_conf->authmode = AUTH_WPA2_PSK;
					} else if (os_strcmp(buffer, "WPAPSK/WPA2PSK") == 0) {
						ap_conf->authmode = AUTH_WPA_WPA2_PSK;
					} else {
						ap_conf->authmode = AUTH_OPEN;
						return 0;
					}
				}

				if (jsonparse_strcmp_value(parser, "channel") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					ap_conf->channel = jsonparse_get_value_as_int(parser);
				} else if (jsonparse_strcmp_value(parser, "ssid") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					jsonparse_copy_value(parser, buffer, sizeof(buffer));
					os_memcpy(ap_conf->ssid, buffer, os_strlen(buffer));
				} else if (jsonparse_strcmp_value(parser, "password") == 0) {
					jsonparse_next(parser);
					jsonparse_next(parser);
					jsonparse_copy_value(parser, buffer, sizeof(buffer));
					os_memcpy(ap_conf->password, buffer, os_strlen(buffer));
				}
			}
		}
	}

	return 0;
}
LOCAL struct jsontree_callback wifi_softap_callback =
		JSONTREE_CALLBACK(wifi_softap_get, wifi_softap_set);

JSONTREE_OBJECT(
		softap_config_tree,
		JSONTREE_PAIR("authmode", &wifi_softap_callback), JSONTREE_PAIR("channel", &wifi_softap_callback), JSONTREE_PAIR("ssid", &wifi_softap_callback), JSONTREE_PAIR("password", &wifi_softap_callback));
JSONTREE_OBJECT(
		softap_ip_tree,
		JSONTREE_PAIR("ip", &wifi_softap_callback), JSONTREE_PAIR("mask", &wifi_softap_callback), JSONTREE_PAIR("gw", &wifi_softap_callback));
JSONTREE_OBJECT(
		get_softap_tree,
		JSONTREE_PAIR("Connect_Softap", &softap_config_tree), JSONTREE_PAIR("Ipinfo_Softap", &softap_ip_tree));
JSONTREE_OBJECT(set_softap_tree,
		JSONTREE_PAIR("Ipinfo_Softap", &softap_config_tree));

JSONTREE_OBJECT(
		get_wifi_tree,
		JSONTREE_PAIR("Station", &get_station_tree), JSONTREE_PAIR("Softap", &get_softap_tree));
JSONTREE_OBJECT(
		set_wifi_tree,
		JSONTREE_PAIR("Station", &set_station_tree), JSONTREE_PAIR("Softap", &set_softap_tree));

JSONTREE_OBJECT(wifi_response_tree, JSONTREE_PAIR("Response", &get_wifi_tree));
JSONTREE_OBJECT(wifi_request_tree, JSONTREE_PAIR("Request", &set_wifi_tree));

JSONTREE_OBJECT(wifi_info_tree, JSONTREE_PAIR("wifi", &wifi_response_tree));
JSONTREE_OBJECT(wifi_req_tree, JSONTREE_PAIR("wifi", &wifi_request_tree));

/******************************************************************************
 * FunctionName : parse_url
 * Description  : parse the received data from the server
 * Parameters   : precv -- the received data
 *                purl_frame -- the result of parsing the url
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
parse_url(char *precv, URL_Frame *purl_frame) {
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *) os_strstr(precv, "Host:");

	if (pbuffer != NULL) {
		length = pbuffer - precv;
		pbufer = (char *) os_zalloc(length + 1);
		pbuffer = pbufer;
		os_memcpy(pbuffer, precv, length);
		os_memset(purl_frame->pSelect, 0, URLSize);
		os_memset(purl_frame->pCommand, 0, URLSize);
		os_memset(purl_frame->pFilename, 0, URLSize);

		if (os_strncmp(pbuffer, "GET ", 4) == 0) {
			purl_frame->Type = GET;
			pbuffer += 4;
		} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
			purl_frame->Type = POST;
			pbuffer += 5;
		}

		pbuffer++;
		str = (char *) os_strstr(pbuffer, "?");

		if (str != NULL) {
			length = str - pbuffer;
			os_memcpy(purl_frame->pSelect, pbuffer, length);
			str++;
			pbuffer = (char *) os_strstr(str, "=");

			if (pbuffer != NULL) {
				length = pbuffer - str;
				os_memcpy(purl_frame->pCommand, str, length);
				pbuffer++;
				str = (char *) os_strstr(pbuffer, "&");

				if (str != NULL) {
					length = str - pbuffer;
					os_memcpy(purl_frame->pFilename, pbuffer, length);
				} else {
					str = (char *) os_strstr(pbuffer, " HTTP");

					if (str != NULL) {
						length = str - pbuffer;
						os_memcpy(purl_frame->pFilename, pbuffer, length);
					}
				}
			}
		}

		os_free(pbufer);
	} else {
		return;
	}
}

LOCAL char *precvbuffer;
static uint32 dat_sumlength = 0;

LOCAL bool ICACHE_FLASH_ATTR
save_data(char *precv, uint16 length) {
	bool flag = false;
	char length_buf[10] = { 0 };
	char *ptemp = NULL;
	char *pdata = NULL;
	uint16 headlength = 0;
	static uint32 totallength = 0;

	ptemp = (char *) os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		length -= ptemp - precv;
		length -= 4;
		totallength += length;
		headlength = ptemp - precv + 4;
		pdata = (char *) os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			precvbuffer = (char *) os_strstr(pdata, "\r\n");

			if (precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
			}
		} else {
			if (totallength != 0x00) {
				totallength = 0;
				dat_sumlength = 0;
				return false;
			}
		}
		if ((dat_sumlength + headlength) >= 1024) {
			precvbuffer = (char *) os_zalloc(headlength + 1);
			os_memcpy(precvbuffer, precv, headlength + 1);
		} else {
			precvbuffer = (char *) os_zalloc(dat_sumlength + headlength + 1);
			os_memcpy(precvbuffer, precv, os_strlen(precv));
		}
	} else {
		if (precvbuffer != NULL) {
			totallength += length;
			os_memcpy(precvbuffer + os_strlen(precvbuffer), precv, length);
		} else {
			totallength = 0;
			dat_sumlength = 0;
			return false;
		}
	}

	if (totallength == dat_sumlength) {
		totallength = 0;
		dat_sumlength = 0;
		return true;
	} else {
		return false;
	}
}

LOCAL bool ICACHE_FLASH_ATTR
check_data(char *precv, uint16 length) {
	char length_buf[10] = { 0 };
	char *ptemp = NULL;
	char *pdata = NULL;
	char *tmp_precvbuffer;
	uint16 tmp_length = length;
	uint32 tmp_totallength = 0;

	ptemp = (char *) os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		tmp_length -= ptemp - precv;
		tmp_length -= 4;
		tmp_totallength += tmp_length;

		pdata = (char *) os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			tmp_precvbuffer = (char *) os_strstr(pdata, "\r\n");

			if (tmp_precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, tmp_precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
				os_printf("A_dat:%u,tot:%u,lenght:%u\n", dat_sumlength,
						tmp_totallength, tmp_length);
				if (dat_sumlength != tmp_totallength) {
					return false;
				}
			}
		}
	}
	return true;
}
LOCAL os_timer_t *restart_10ms;
LOCAL rst_parm *rstparm;

/******************************************************************************
 * FunctionName : restart_10ms_cb
 * Description  : system restart or wifi reconnected after a certain time.
 * Parameters   : arg -- Additional argument to pass to the function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
restart_10ms_cb(void *arg) {
	if (rstparm != NULL && rstparm->pespconn != NULL) {
		switch (rstparm->parmtype) {
		case WIFI:
			if (sta_conf->ssid[0] != 0x00) {
				CFG_saveStaCfg(*sta_conf,true);
				user_set_com_mode();
			}
			if (ap_conf->ssid[0] != 0x00) {
				CFG_saveSoftApPass(ap_conf->password, true);
				user_set_com_mode();
			}

			os_free(ap_conf);
			ap_conf = NULL;
			os_free(sta_conf);
			sta_conf = NULL;
			os_free(rstparm);
			rstparm = NULL;
			os_free(restart_10ms);
			restart_10ms = NULL;

			break;
		case REBOOT:
			if (rstparm->pespconn->state == ESPCONN_CLOSE) {
				wifi_set_opmode(STATION_MODE);
			} else {
				os_timer_arm(restart_10ms, 10, 0);
			}

			break;
		default:
			break;
		}
	}
}

/******************************************************************************
 * FunctionName : data_send
 * Description  : processing the data as http format and send to the client or server
 * Parameters   : arg -- argument to set for client or server
 *                responseOK -- true or false
 *                psend -- The send data
 * Returns      :
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
data_send(void *arg, bool responseOK, char *psend) {
	uint16 length = 0;
	char *pbuf = NULL;
	char httphead[256];
	struct espconn *ptrespconn = arg;
	os_memset(httphead, 0, 256);

	if (responseOK) {
		os_sprintf(
				httphead,
				"HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: lwIP/1.4.0\r\n",
				psend ? os_strlen(psend) : 0);

		if (psend) {
			os_sprintf(
					httphead + os_strlen(httphead),
					"Content-type: application/json\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
			length = os_strlen(httphead) + os_strlen(psend);
			pbuf = (char *) os_zalloc(length + 1);
			os_memcpy(pbuf, httphead, os_strlen(httphead));
			os_memcpy(pbuf + os_strlen(httphead), psend, os_strlen(psend));
		} else {
			os_sprintf(httphead + os_strlen(httphead), "\n");
			length = os_strlen(httphead);
		}
	} else {
		os_sprintf(
				httphead,
				"HTTP/1.0 400 BadRequest\r\n\
Content-Length: 0\r\nServer: lwIP/1.4.0\r\n\n");
		length = os_strlen(httphead);
	}

	if (psend) {
#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, pbuf, length);
#else
		espconn_sent(ptrespconn, pbuf, length);
#endif
	} else {
#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, httphead, length);
#else
		espconn_sent(ptrespconn, httphead, length);
#endif
	}

	if (pbuf) {
		os_free(pbuf);
		pbuf = NULL;
	}
}

/******************************************************************************
 * FunctionName : json_send
 * Description  : processing the data as json format and send to the client or server
 * Parameters   : arg -- argument to set for client or server
 *                ParmType -- json format type
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
json_send(void *arg, ParmType ParmType) {
	char *pbuf = NULL;
	pbuf = (char *) os_zalloc(jsonSize);
	struct espconn *ptrespconn = arg;

	switch (ParmType) {

	case WIFI:
		json_ws_send((struct jsontree_value *) &wifi_info_tree, "wifi", pbuf);
		break;
	case CONNECT_STATUS:
		json_ws_send((struct jsontree_value *)&con_status_tree, "info", pbuf);
		break;
	case MESH:
		json_ws_send((struct jsontree_value *)&mesh_config_resp, "MeshResp", pbuf);
		break;
	case DEVICE_CFG:
		json_ws_send((struct jsontree_value *)&device_cfg_resp, "DeviceCfgResp", pbuf);
		break;
	case MQTT:
		json_ws_send((struct jsontree_value *)&mqtt_info_resp, "MqttInfoReq", pbuf);
		break;
	default:
		break;
	}

	data_send(ptrespconn, true, pbuf);
	os_free(pbuf);
	pbuf = NULL;
}

/******************************************************************************
 * FunctionName : response_send
 * Description  : processing the send result
 * Parameters   : arg -- argument to set for client or server
 *                responseOK --  true or false
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
response_send(void *arg, bool responseOK) {
	struct espconn *ptrespconn = arg;

	data_send(ptrespconn, responseOK, NULL);
}


/******************************************************************************
 * FunctionName : webserver_recv
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	bool parse_flag = false;
	struct espconn *ptrespconn = arg;

	os_printf("len:%u\n", length);
	if (check_data(pusrdata, length) == false)
	{
		os_printf("goto\n");
		goto _temp_exit;
	}

	parse_flag = save_data(pusrdata, length);
	if (parse_flag == false) {
		response_send(ptrespconn, false);
	}

	pURL_Frame = (URL_Frame *) os_zalloc(sizeof(URL_Frame));
	parse_url(precvbuffer, pURL_Frame);

	switch (pURL_Frame->Type) {
	case GET:
		os_printf("We have a GET request.\n");

		if (os_strcmp(pURL_Frame->pSelect, "client") == 0
				&& os_strcmp(pURL_Frame->pCommand, "command") == 0) {
			if (os_strcmp(pURL_Frame->pFilename, "status") == 0) {//curl -X GET http://192.168.11.140/client?command=status
				json_send(ptrespconn, CONNECT_STATUS);
			} else {
				response_send(ptrespconn, false);
			}
		} else if (os_strcmp(pURL_Frame->pSelect, "config") == 0
				&& os_strcmp(pURL_Frame->pCommand, "command") == 0) {
			if (os_strcmp(pURL_Frame->pFilename, "wifi") == 0) {//curl -X GET http://192.168.11.140/config?command=wifi
				ap_conf =
						(struct softap_config *) os_zalloc(sizeof(struct softap_config));
				sta_conf =
						(struct station_config *) os_zalloc(sizeof(struct station_config));
				json_send(ptrespconn, WIFI);
				os_free(sta_conf);
				os_free(ap_conf);
				sta_conf = NULL;
				ap_conf = NULL;
			} else if (os_strcmp(pURL_Frame->pFilename, "mesh") == 0) {//curl -X GET http://192.168.11.140/config?command=mesh
				json_send(ptrespconn, MESH);
			} else if (os_strcmp(pURL_Frame->pFilename, "device_cfg") == 0) {//curl -X GET http://192.168.11.140/config?command=mesh
				json_send(ptrespconn, DEVICE_CFG);
			} else if (os_strcmp(pURL_Frame->pFilename, "mqtt") == 0) {//curl -X GET http://192.168.11.140/config?command=mesh
				json_send(ptrespconn, MQTT);
			} else {
				response_send(ptrespconn, false);
			}
		} else {
			response_send(ptrespconn, false);
		}

		break;

	case POST:
		os_printf("We have a POST request.\n");
		pParseBuffer = (char *) os_strstr(precvbuffer, "\r\n\r\n");

		if (pParseBuffer == NULL) {
			break;
		}

		pParseBuffer += 4;

		if (os_strcmp(pURL_Frame->pSelect, "config") == 0
				&& os_strcmp(pURL_Frame->pCommand, "command") == 0) {

			if (os_strcmp(pURL_Frame->pFilename, "reboot") == 0) {

				if (pParseBuffer != NULL) {
					if (restart_10ms != NULL) {
						os_timer_disarm(restart_10ms);
					}

					if (rstparm == NULL) {
						rstparm = (rst_parm *) os_zalloc(sizeof(rst_parm));
					}

					rstparm->pespconn = ptrespconn;

					rstparm->parmtype = REBOOT;

					if (restart_10ms == NULL) {
						restart_10ms =
								(os_timer_t *) os_malloc(sizeof(os_timer_t));
					}

					os_timer_setfn(restart_10ms,
							(os_timer_func_t *) restart_10ms_cb, NULL);
					os_timer_arm(restart_10ms, 10, 0);
					// delay 10ms, then do

					response_send(ptrespconn, true);
				} else {
					response_send(ptrespconn, false);
				}
			} else if (os_strcmp(pURL_Frame->pFilename, "wifi") == 0) {
				if (pParseBuffer != NULL) {
					struct jsontree_context js;
					user_esp_platform_set_connect_status(DEVICE_CONNECTING);
					if (restart_10ms != NULL) {
						os_timer_disarm(restart_10ms);
					}

					if (ap_conf == NULL) {
						ap_conf =
								(struct softap_config *) os_zalloc(sizeof(struct softap_config));
					}

					if (sta_conf == NULL) {
						sta_conf =
								(struct station_config *) os_zalloc(sizeof(struct station_config));
					}

					jsontree_setup(&js,
							(struct jsontree_value *) &wifi_req_tree,
							json_putchar);
					json_parse(&js, pParseBuffer);

					if (rstparm == NULL) {
						rstparm = (rst_parm *) os_zalloc(sizeof(rst_parm));
					}

					rstparm->pespconn = ptrespconn;
					rstparm->parmtype = WIFI;

					if (sta_conf->ssid[0] != 0x00 || ap_conf->ssid[0] != 0x00) {
						ap_conf->ssid_hidden = 0;
						ap_conf->max_connection = 4;

						if (restart_10ms == NULL) {
							restart_10ms =
									(os_timer_t *) os_malloc(sizeof(os_timer_t));
						}

						os_timer_disarm(restart_10ms);
						os_timer_setfn(restart_10ms,
								(os_timer_func_t *) restart_10ms_cb, NULL);
						os_timer_arm(restart_10ms, 10, 0);
						// delay 10ms, then do
					} else {
						os_free(ap_conf);
						os_free(sta_conf);
						os_free(rstparm);
						sta_conf = NULL;
						ap_conf = NULL;
						rstparm = NULL;
					}

					response_send(ptrespconn, true);
				} else {
					response_send(ptrespconn, false);
				}
			} else if (os_strcmp(pURL_Frame->pFilename, "mesh") == 0) {
				if (pParseBuffer != NULL) {
					struct jsontree_context js;
					jsontree_setup(&js,(struct jsontree_value *) &mesh_config_req,json_putchar);
					json_parse(&js, pParseBuffer);
					response_send(ptrespconn, true);
				} else {
					response_send(ptrespconn, false);
				}
			} else if (os_strcmp(pURL_Frame->pFilename, "device_cfg") == 0) {
				if (pParseBuffer != NULL) {
					struct jsontree_context js;
					jsontree_setup(&js,(struct jsontree_value *) &device_cfg_req,json_putchar);
					json_parse(&js, pParseBuffer);
					response_send(ptrespconn, true);
				} else {
					response_send(ptrespconn, false);
				}
			} else if (os_strcmp(pURL_Frame->pFilename, "mqtt") == 0) {
				if (pParseBuffer != NULL) {
					struct jsontree_context js;
					jsontree_setup(&js,(struct jsontree_value *) &mqtt_info_req,json_putchar);
					json_parse(&js, pParseBuffer);
					response_send(ptrespconn, true);
				} else {
					response_send(ptrespconn, false);
				}
			} else {
				response_send(ptrespconn, false);
			}
		} else {
			response_send(ptrespconn, false);
		}
		break;
	}

	if (precvbuffer != NULL) {
		os_free(precvbuffer);
		precvbuffer = NULL;
	}
	os_free(pURL_Frame);
	pURL_Frame = NULL;
	_temp_exit: ;
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void webserver_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n",
			pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port, err);
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void webserver_discon(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n",
			pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port);

}

/******************************************************************************
 * FunctionName : user_accept_listen
 * Description  : server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
webserver_listen(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, webserver_recv);
	espconn_regist_reconcb(pesp_conn, webserver_recon);
	espconn_regist_disconcb(pesp_conn, webserver_discon);
}






void ICACHE_FLASH_ATTR
user_webserver_start(uint32 port) {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, webserver_listen);


#ifdef SERVER_SSL_ENABLE
	espconn_secure_set_default_certificate(default_certificate, default_certificate_len);
	espconn_secure_set_default_private_key(default_private_key, default_private_key_len);
	espconn_secure_accept(&esp_conn);
#else
	espconn_accept(&esp_conn);
#endif
}


/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(void) {
	struct ip_info sta_ipconfig;
	struct ip_info ap_ipconfig;

	//disarm timer first
	os_timer_disarm(&test_timer);

	//get ip info of ESP8266 station
	wifi_get_ip_info(STATION_IF, &sta_ipconfig);
	wifi_get_ip_info(SOFTAP_IF, &ap_ipconfig);

	if (sta_ipconfig.ip.addr != 0 || ap_ipconfig.ip.addr != 0) {

		if(sta_ipconfig.ip.addr != 0){
			local_ip.addr = sta_ipconfig.ip.addr;
		}else if(ap_ipconfig.ip.addr != 0){
			local_ip.addr = ap_ipconfig.ip.addr;
		}
		//os_printf("got ip !!! \r\n");
		//os_printf(IPSTR, IP2STR(&local_ip.ip));
		user_webserver_start(SERVER_PORT);

	} else {
		//re-arm timer to check ip
		os_timer_setfn(&test_timer,
				(os_timer_func_t *) user_esp_platform_check_ip, NULL);
		os_timer_arm(&test_timer, 100, 0);
	}
}


/******************************************************************************
 * FunctionName : user_webserver_init
 * Description  : parameter initialize as a server
 * Parameters   : port -- server port
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_webserver_init(user_webserverCallback onDataChangedCb) {
	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer, (os_timer_func_t *) user_esp_platform_check_ip,
	NULL);
	os_timer_arm(&test_timer, 100, 0);
	onDataChangedCbFunc = onDataChangedCb;
}
