/******************************************************************************
 * Copyright 2015-2016 Espressif Systems
 *
 * FileName: mesh_json.c
 *
 * Description: mesh demo for JSON protocol parser
 *
 * Modification history:
 *     2016/03/15, v1.1 create this file.
 *******************************************************************************/
#include "mem.h"
#include "osapi.h"
#include "master_mesh_parser.h"
#include "user_interface.h"
#include "user_mqtt.h"
//#include "user_json.h"
#include "slaveMeterList.h"


void ICACHE_FLASH_ATTR
mesh_json_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len) {
	struct mesh_header_format *header =
			(struct mesh_header_format *) mesh_header;
	//MESH_PARSER_PRINT("%s\n", __func__);
	MESH_PARSER_PRINT("len:%u, original len: %d, data:%s\n", len, os_strlen(pdata), pdata);
	//os_printf("src mac: " MACSTR "\n", MAC2STR(header->src_addr));
	//os_printf("dst mac: " MACSTR "\n", MAC2STR(header->dst_addr));
	//user_parse_json_to_meter_data(pdata, meter_data);
	add_unique_meter_update_data(header->src_addr);
	find_delete_inactive_meter(40000);
	/*if (user_mqtt_connected() && meter_data->data_len>0) {
		char *data = (char *)os_zalloc((sizeof(char)*meter_data->data_len)+30);
		os_sprintf(data, MACSTR ": %s", MAC2STR(header->src_addr), meter_data->data);
		user_mqtt_publish("E/1/test", data);
		os_free(data);
	}
	else{
		os_printf("\r\n invalid data with len = %d or mqt is %d\r\n",meter_data->data_len,user_mqtt_connected());
	}*/
}
