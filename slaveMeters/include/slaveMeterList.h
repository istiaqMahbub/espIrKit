/*
 * slaveMeterList.h
 *
 *  Created on: Oct 4, 2016
 *      Author: kanok
 */

#ifndef SLAVEMETERS_INCLUDE_SLAVEMETERLIST_H_
#define SLAVEMETERS_INCLUDE_SLAVEMETERLIST_H_

#include "c_types.h"

#define MESH_ADDR_LEN 6

struct meter_t {
	uint8_t mac[MESH_ADDR_LEN];
	bool is_sub_acked;
	bool is_sub_posted;
	bool is_unsub_posted;
	bool is_live;
	uint32 last_seen;
}__packed;
//why this symbol used?????

struct slave_meter_list_t {
	struct meter_t meter;
	uint8_t key;
	struct slave_meter_list_t *next;
	struct slave_meter_list_t *prev;
	//mesh_gateway_ip;
	//mesh_gateway_port;
};

#endif /* SLAVEMETERS_INCLUDE_SLAVEMETERLIST_H_ */
