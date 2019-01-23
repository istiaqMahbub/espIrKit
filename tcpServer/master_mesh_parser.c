/******************************************************************************
 * Copyright 2015-2016 Espressif Systems
 *
 * FileName: mesh_parser.c
 *
 * Description: mesh demo for general packet parser
 *
 * Modification history:
 *     2016/03/15, v1.1 create this file.
 *******************************************************************************/
#include "master_mesh_parser.h"
#include "osapi.h"
#include "user_interface.h"
#include "slaveMeterList.h"
#include "mem.h"
#include "espconn.h"

static struct mesh_general_parser_type g_packet_parser[] = { { M_PROTO_NONE,
		mesh_none_proto_parser }, { M_PROTO_HTTP, mesh_http_proto_parser }, {
		M_PROTO_JSON, mesh_json_proto_parser }, { M_PROTO_MQTT,
		mesh_mqtt_proto_parser }, { M_PROTO_BIN, mesh_bin_proto_parser } };

//LOCAL struct mesh_header_format *headerTemplate = NULL;
//static struct espconn *pespconn = NULL;

bool ICACHE_FLASH_ATTR
apl_mesh_get_usr_data(struct mesh_header_format *head, uint8_t *usr_data,
		uint8_t *pdata, uint16_t *data_len) {
	if (head->proto.protocol <= 4 && head->len > 16) {
		*data_len = head->len - 16;
		os_memcpy(usr_data, pdata + 16, head->len - 16);
		usr_data[head->len - 16] = 0;
		return true;
	}
	return false;
}

bool ICACHE_FLASH_ATTR
apl_mesh_get_usr_data_proto(struct mesh_header_format *head,
		enum mesh_usr_proto_type *proto) {
	if (head->proto.protocol <= 4) {
		*proto = head->proto.protocol;
		return true;
	}
	return false;
}

void ICACHE_FLASH_ATTR
mesh_packet_parser(void *arg, uint8_t *pdata, uint16_t len) {
	uint16_t i = 0;
	uint8_t usr_data[100];
	uint16_t usr_data_len = 0;
	enum mesh_usr_proto_type proto;
	struct mesh_header_format *header = (struct mesh_header_format *) pdata;
	uint16_t proto_count = sizeof(g_packet_parser) / sizeof(g_packet_parser[0]);
	if (!apl_mesh_get_usr_data_proto(header, &proto))
		return;
	if (!apl_mesh_get_usr_data(header, usr_data, pdata, &usr_data_len)) {
		// mesh topology packet
		//os_memcpy(usr_data, pdata, len);
		//usr_data_len = len;
	}
	for (i = 0; i < proto_count; i++) {
		if (g_packet_parser[i].proto == proto) {
			if (g_packet_parser[i].handler == NULL)
				break;
			g_packet_parser[i].handler(header, usr_data, usr_data_len);
			break;
		}
	}
}

bool ICACHE_FLASH_ATTR
mesh_create_packet(uint8_t *mesh_packet, uint8_t *mesh_packet_len,
		uint8_t *src_address, uint8_t *dst_address, uint8_t *data,
		uint8_t data_len) {
		struct mesh_header_format *header = (struct mesh_header_format *)os_zalloc((sizeof(struct mesh_header_format)));
		os_memcpy(header->dst_addr, dst_address, 6);
		os_memset(header->src_addr, 0, 6);
		os_printf("dst mac:" MACSTR "\n", MAC2STR(header->dst_addr));
		os_printf("src mac:" MACSTR "\n", MAC2STR(header->src_addr));
		header->ver = 0;
		header->oe = 0; // option exist flag
		header->cp = 0; // option exist flag
		header->cr = 0; // piggyback congest request in packet
		header->proto.d = 0; // direction, 1:upwards, 0:downwards
		header->proto.p2p = 0;
		header->proto.protocol = M_PROTO_JSON;
		header->len = data_len+16;
		os_memcpy(mesh_packet, header, sizeof(struct mesh_header_format));
		os_memcpy(&mesh_packet[sizeof(struct mesh_header_format)], data,
				data_len);
		*mesh_packet_len = (sizeof(struct mesh_header_format) + data_len);
		os_free(header);
		return true;
}

bool ICACHE_FLASH_ATTR
mesh_create_send_packet(uint8_t *dst_address, uint8_t *data, uint8_t data_len) {
	uint8_t src_address[6];
	uint8_t *mesh_packet = (uint8_t*) os_zalloc(
					(sizeof(struct mesh_header_format) + data_len));
	uint8_t mesh_packet_len;
	bool is_success = false;
	is_success = mesh_create_packet(mesh_packet, &mesh_packet_len, src_address,
			dst_address, data, data_len);
	if (is_success) {
		is_success = mesh_send_packet(mesh_packet, mesh_packet_len);
	}
	os_free(mesh_packet);
	return is_success;
}
