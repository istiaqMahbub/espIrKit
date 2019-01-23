/*
 * slaveMeterList.c
 *
 *  Created on: Oct 4, 2016
 *  Author: Istiaq Mahbub
 */
#include "slaveMeterList.h"
#include "osapi.h"
#include "user_interface.h"
#include <mem.h>
#include "user_mqtt.h"

struct slave_meter_list_t *head = NULL;

//is list empty
bool ICACHE_FLASH_ATTR
isEmpty() {
	return head == NULL;
}

//insert link at the first location
struct slave_meter_list_t* ICACHE_FLASH_ATTR
insertFirst(uint8_t *mac) {
	//create a link
	struct slave_meter_list_t *link = (struct slave_meter_list_t*) os_malloc(
			sizeof(struct slave_meter_list_t));

	os_memcpy(link->meter.mac, mac, MESH_ADDR_LEN);
	link->meter.is_live = true;
	link->meter.is_sub_acked = false;
	link->meter.last_seen = system_get_time();
	link->key = length();
	if (isEmpty()) {
		//make it the last link
		//last = link;
	} else {
		//update first prev link
		head->prev = link;
	}

	//point it to old first link
	link->next = head;
	link->prev = NULL;

	//point first to new first link
	head = link;
	os_printf("\r\nNew mac inserted\r\n");
	return head;
}

int ICACHE_FLASH_ATTR
length() {
	int length = 0;
	struct slave_meter_list_t *current;

	for (current = head; current != NULL; current = current->next) {
		length++;
	}

	return length;
}

bool ICACHE_FLASH_ATTR
cmp_mac(uint8_t *mac1, uint8_t *mac2) {
	uint8_t i = 0;
	for (i = 0; i < MESH_ADDR_LEN; i++) {
		if (mac1[i] != mac2[i]) {
			return false;
		}
	}
	return true;
}

//find a link with given key
struct slave_meter_list_t* ICACHE_FLASH_ATTR
find_meter(uint8_t *mac) {

	if (mac == NULL)
		return NULL;
	//start from the first link
	struct slave_meter_list_t* current = head;

	//if list is empty
	if (head == NULL) {
		return NULL;
	}

	//navigate through list
	while (!cmp_mac(mac, current->meter.mac)) {

		//if it is last node
		if (current->next == NULL) {
			return NULL;
		} else {
			//go to next link
			current = current->next;
		}
	}

	//if data found, return the current Link
	return current;
}

bool ICACHE_FLASH_ATTR
delete_all() {
	head = NULL;
	return true;
}

bool ICACHE_FLASH_ATTR
delete_by_meter(struct slave_meter_list_t *meter) {
	if (head == NULL || meter == NULL) {
		return false;
	}
	struct slave_meter_list_t* previous = meter->prev;
	struct slave_meter_list_t* next = meter->next;
	if (previous != NULL && meter->next != NULL)	      // if in the middle
	{
		previous->next = meter->next;
		next->prev = previous;
	} else if (previous == NULL && meter->next != NULL)	//deleting from left end
	{
		next->prev = NULL;
		head = next;
	} else if (previous != NULL && meter->next == NULL)	//deleting from right end
	{
		previous->next = NULL;
	} else {
		head = NULL;
	}
	return true;
}

//delete a link with given key
struct slave_meter_list_t* ICACHE_FLASH_ATTR
delete_by_mac(uint8_t *mac) {

	//if list is empty
	if (head == NULL) {
		return NULL;
	}

	struct slave_meter_list_t *meter = (struct slave_meter_list_t *) find_meter(
			mac);
	delete_by_meter(meter);

	return meter;
}

//display the list
void ICACHE_FLASH_ATTR
printList() {
	struct slave_meter_list_t *ptr = head;
	os_printf("\r\n=========mac list=============\r\n");

	//start from the beginning
	uint8_t i = 0;
	while (ptr != NULL) {
		if (system_get_time() > ptr->meter.last_seen + 10000000) {
			delete_by_meter(ptr);
			os_printf("deleting mac[%d]:" MACSTR "\n", i,
					MAC2STR(ptr->meter.mac));
		} else {
			os_printf("src mac[%d]:" MACSTR "\n", i,
					MAC2STR(ptr->meter.mac));
		}
		ptr = ptr->next;
		i++;
	}

	os_printf("\r\n=========mac list end==========\r\n");
}

bool ICACHE_FLASH_ATTR
add_unique_meter_update_data(uint8_t *mac) {
	struct slave_meter_list_t *meter = (struct slave_meter_list_t *) find_meter(
			mac);
	bool is_added = false;
	if (meter == NULL) {
		meter = insertFirst(mac);
		is_added = true;
		if (user_mqtt_connected()) {
			meter->meter.is_sub_posted =
					user_mqtt_subscribe_by_non_string_topic(mac, 0);
			os_printf(
					"\r\n######New meter added and sub posted########" MACSTR "\r\n",
					MAC2STR(mac));
		}
	} else {
		meter->meter.is_live = true;
		meter->meter.last_seen = system_get_time();
		if (user_mqtt_connected()) {
			if (!meter->meter.is_sub_posted) {
				meter->meter.is_sub_posted =
						user_mqtt_subscribe_by_non_string_topic(mac, 0);
				os_printf(
						"\r\n######Old meter Re sub posted########" MACSTR "\r\n",
						MAC2STR(mac));
			} else {

			}
		} else {
			if (meter->meter.is_sub_posted) {
				meter->meter.is_sub_posted = false;
				os_printf(
						"\r\n######Old meter going offline########" MACSTR "\r\n",
						MAC2STR(mac));
			}
		}
	}
	return is_added;
}
bool ICACHE_FLASH_ATTR
find_delete_create_inactive_meter_list(int timeout_ms, uint8_t *listData,
		uint16_t *len) {
	struct slave_meter_list_t *ptr = head;
	os_printf("\r\n=========mac list in %s=============\r\n", __func__);
	//start from the beginning
	uint8_t i = 0;
	uint8_t idx = 0;
	bool is_list_created = false;
	timeout_ms *= 1000;
	while (ptr != NULL) {
		if (system_get_time() > ptr->meter.last_seen + timeout_ms) {
			delete_by_meter(ptr);
			os_printf("deleting mac[%d]:" MACSTR "\n", i,
					MAC2STR(ptr->meter.mac));
			os_memcpy(listData + idx, ptr->meter.mac, MESH_ADDR_LEN);
			idx += MESH_ADDR_LEN;
			is_list_created = true;
		} else {
			os_printf("Active src mac[%d]:" MACSTR "\n", i,
					MAC2STR(ptr->meter.mac));
		}
		ptr = ptr->next;
		i++;
	}
	*len = idx;
	os_printf("\r\n=========mac list end in %s==========\r\n", __func__);
	return is_list_created;
}

uint8_t ICACHE_FLASH_ATTR
find_delete_inactive_meter(int timeout_ms) {
	struct slave_meter_list_t *ptr = head;
	//os_printf("\r\n=========mac list in %s=============\r\n",__func__);
	//start from the beginning
	uint8_t i = 0;
	uint8_t idx = 0;
	timeout_ms *= 1000;
	while (ptr != NULL) {
		if (system_get_time() > ptr->meter.last_seen + timeout_ms) {
			if (user_mqtt_connected()) {
				ptr->meter.is_unsub_posted =
						user_mqtt_unsubscribe_by_non_string_topic(
								ptr->meter.mac);
				if (ptr->meter.is_unsub_posted) {
					delete_by_meter(ptr);
					os_printf("deleting mac[%d]:" MACSTR "\n", i,
							MAC2STR(ptr->meter.mac));
					idx++;
				}
			} else {
				delete_by_meter(ptr);
				os_printf("deleting mac[%d]:" MACSTR "\n", i,
						MAC2STR(ptr->meter.mac));
				idx++;
			}
		} else {
			//MESH_PARSER_PRINT("Active src mac[%d]:" MACSTR ", MQTT_CON = %d\n", i, MAC2STR(ptr->meter.mac),user_mqtt_connected());
		}
		ptr = ptr->next;
		i++;
	}
	//os_printf("\r\n=========mac list end in %s==========\r\n",__func__);
	return idx;
}



