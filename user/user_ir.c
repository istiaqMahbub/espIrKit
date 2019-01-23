/*
 * user_ir.c
 *
 * Created on: Aug 7, 2017
 * Author: Istiaq Mahbub
 */
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "ip_addr.h"
#include "user_json.h"
#include "config.h"
#include "user_ir.h"
#include "base64.h"
#include "user_uart.h"


LOCAL uint8_t cmd;
LOCAL uint8_t id_cmd;
LOCAL uint16_t token;
LOCAL uint16_t len_data;
LOCAL uint8_t data[1024];
LOCAL user_irCmdCallback onIrCmdAvailable = NULL;


LOCAL bool ICACHE_FLASH_ATTR
get_sta_ip(ip_addr_t *local_ip){
	struct ip_info sta_ipconfig;
	wifi_get_ip_info(STATION_IF, &sta_ipconfig);
	if (sta_ipconfig.ip.addr != 0) {
		local_ip->addr = sta_ipconfig.ip.addr;
		return TRUE;
	}else{
		return FALSE;
	}
}

//step 4: define callback function for get/set

/******************************************************************************
 * FunctionName : wifi_station_get
 * Description  : set up the station paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
 *******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
ir_info_to_publish(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	uint8 buf[64];
	os_bzero(buf, sizeof(buf));
	if (os_strncmp(path, "local_ip", 8) == 0) {
		ip_addr_t local_ip;
		get_sta_ip(&local_ip);
		os_sprintf(buf, IPSTR, IP2STR(&local_ip));
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "strId", 5) == 0) {
		os_sprintf(buf, "%s", sysCfg.device_id);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "cmd", 3) == 0) {
		os_sprintf(buf, "%d", cmd);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "id_cmd", 6) == 0) {
		os_sprintf(buf, "%d", id_cmd);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "token", 5) == 0) {
		os_sprintf(buf, "%d", token);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "len_data", 8) == 0) {
		os_sprintf(buf, "%d", len_data);
		jsontree_write_string(js_ctx, buf);
	} else if (os_strncmp(path, "data", 4) == 0) {
		//os_printf("%s", data);
		jsontree_write_string(js_ctx, data);
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
ir_info_subscribed(struct jsontree_context *js_ctx,
		struct jsonparse_state *parser) {
	int type;
	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			char buffer[64];
			os_bzero(buffer, sizeof(buffer));
			if (jsonparse_strcmp_value(parser, "sender_ip") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				os_printf("sender_ip: %s\r\n",buffer);
			} else if (jsonparse_strcmp_value(parser, "cmd") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				cmd = Atoi(buffer);
				os_printf("cmd: %s\r\n",buffer);
			} else if (jsonparse_strcmp_value(parser, "id_cmd") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				id_cmd = Atoi(buffer);
				os_printf("id_cmd: %s\r\n",buffer);
			} else if (jsonparse_strcmp_value(parser, "token") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				token = Atoi(buffer);
				os_printf("token: %s\r\n",buffer);
			} else if (jsonparse_strcmp_value(parser, "len_data") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, buffer, sizeof(buffer));
				len_data = Atoi(buffer);
				os_printf("len_data: %s\r\n",buffer);
			} else if (jsonparse_strcmp_value(parser, "data") == 0) {
				os_bzero(data,sizeof(data));
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, data, sizeof(data));
				//os_printf("data: %s\r\n",data);
			}
		}
	}
	if(onIrCmdAvailable!=NULL){
		onIrCmdAvailable(cmd,id_cmd,data, len_data);
	}
	os_printf("done ir subscription parsing\r\n");
	return 0;
}





//step 3: declare callback function for get/set
LOCAL struct jsontree_callback ir_info_callback =
		JSONTREE_CALLBACK(ir_info_to_publish, ir_info_subscribed);
//step 3: end


//step 2: define get set tree
JSONTREE_OBJECT(
		publish_ir_info_tree,
		JSONTREE_PAIR("local_ip", &ir_info_callback),\
		JSONTREE_PAIR("strId", &ir_info_callback),\
		JSONTREE_PAIR("dev_com_mode", &ir_info_callback),\
		JSONTREE_PAIR("cmd", &ir_info_callback),\
		JSONTREE_PAIR("id_cmd", &ir_info_callback),\
		JSONTREE_PAIR("token", &ir_info_callback),\
		JSONTREE_PAIR("len_data", &ir_info_callback),\
		JSONTREE_PAIR("data", &ir_info_callback),\
		);

JSONTREE_OBJECT(
		subscribe_ir_info_tree,
		JSONTREE_PAIR("sender_ip", &ir_info_callback),\
		JSONTREE_PAIR("cmd", &ir_info_callback),\
		JSONTREE_PAIR("id_cmd", &ir_info_callback),\
		JSONTREE_PAIR("token", &ir_info_callback),\
		JSONTREE_PAIR("len_data", &ir_info_callback),\
		JSONTREE_PAIR("data", &ir_info_callback),\
		);
///////step 2: end

//step 1: define root tree
JSONTREE_OBJECT(
		ir_cmd_publish,
		JSONTREE_PAIR("ir_fb", &publish_ir_info_tree));

JSONTREE_OBJECT(
		ir_cmd_subscribe,
		JSONTREE_PAIR("ir_cmd", &subscribe_ir_info_tree));
//step 1: end


void ICACHE_FLASH_ATTR
user_ir_json_create(char *pbuf,uint8_t _cmd,uint8_t _id_cmd,uint16_t _len_data,uint8_t *_data){
	cmd = _cmd;
	id_cmd = _id_cmd;
	len_data = _len_data;
	os_bzero(data,sizeof(data));
	os_memcpy(data,_data,len_data);
	json_ws_send((struct jsontree_value *) &ir_cmd_publish, "ir_cmd", pbuf);
}

void ICACHE_FLASH_ATTR
user_ir_json_parse(uint8_t *data){
	struct jsontree_context js;
	jsontree_setup(&js,(struct jsontree_value *) &ir_cmd_subscribe,json_putchar);
	json_parse(&js, data);
}

void ICACHE_FLASH_ATTR
user_ir_cmd_init(user_irCmdCallback onIrCmdCb){
	onIrCmdAvailable = onIrCmdCb;
	cmd = 0;
	id_cmd = 0;
	token = 0;
	len_data = 0;
}



