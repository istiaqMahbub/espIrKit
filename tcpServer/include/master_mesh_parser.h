#ifndef __MESH_PARSER_H__
#define __MESH_PARSER_H__

#include "master_mesh_http.h"
#include "master_mesh_json.h"
#include "master_mesh_mqtt.h"
#include "master_mesh_none.h"
#include "master_mesh_bin.h"


enum mesh_usr_proto_type {
    M_PROTO_NONE = 0,           // used to delivery mesh management packet
    M_PROTO_HTTP,               // user data formated with HTTP protocol
    M_PROTO_JSON,               // user data formated with JSON protocol
    M_PROTO_MQTT,               // user data formated with MQTT protocol
    M_PROTO_BIN,                // user data is binary stream
};


#define ESP_MESH_ADDR_LEN 6
struct mesh_header_option_format {
    uint8_t otype;      // option type
    uint8_t olen;       // current option length
    uint8_t ovalue[0];  // option value
} __packed;

struct mesh_header_option_header_type {
    uint16_t ot_len;    // option total length;
    struct mesh_header_option_format olist[0];  // option list
} __packed;
struct mesh_header_format {
    uint8_t ver:2;          // version of mesh
    uint8_t oe: 1;          // option exist flag
    uint8_t cp: 1;          // piggyback congest permit in packet
    uint8_t cr: 1;          // piggyback congest request in packet
    uint8_t rsv:3;          // reserve for fulture;
    struct {
        uint8_t d:  1;      // direction, 1:upwards, 0:downwards
        uint8_t p2p:1;      // node to node packet
        uint8_t protocol:6; // protocol used by user data;
    } proto;
    uint16_t len;           // packet total length (include mesh header)
    uint8_t dst_addr[ESP_MESH_ADDR_LEN];  // destiny address
    uint8_t src_addr[ESP_MESH_ADDR_LEN];  // source address
    struct mesh_header_option_header_type option[0];  // mesh option
} __packed;

#define MESH_PARSER_DEBUG
#ifdef MESH_PARSER_DEBUG
#define MESH_PARSER_PRINT ets_printf
#else
#define MESH_PARSER_PRINT
#endif

typedef void (*mesh_proto_parser_handler)(const void *mesh_header, uint8_t *pdata, uint16_t len);

struct mesh_general_parser_type {
    uint8_t proto;
    mesh_proto_parser_handler handler;
};

bool mesh_create_packet(uint8_t *mesh_packet, uint8_t *mesh_packet_len, uint8_t *src_address, uint8_t *dst_address, uint8_t *data, uint8_t data_len);
bool mesh_create_send_packet(uint8_t *dst_address, uint8_t *data, uint8_t data_len);
#endif
