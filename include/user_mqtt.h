/*
 * user_mqtt.h
 *
 *  Created on: Oct 5, 2016
 *  Author: Istiaq Mahbub
 */

#ifndef INCLUDE_USER_MQTT_H_
#define INCLUDE_USER_MQTT_H_

#include "mqtt.h"

#define MID2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MIDSTR "%02x%02x%02x%02x%02x%02x"


void user_init_mqtt(MQTT_Client *mqttClient);
void mqttConnectedCb(uint32_t *args);
void mqttDisconnectedCb(uint32_t *args);
void mqttPublishedCb(uint32_t *args);
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len,const char *data, uint32_t data_len);
bool getMqttStatus();


#endif /* INCLUDE_USER_MQTT_H_ */
