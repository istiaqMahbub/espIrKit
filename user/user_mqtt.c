/*
 * user_mqtt.c
 *
 * Created on: Oct 5, 2016
 * Author: Istiaq Mahbub
 */
#include "ets_sys.h"
#include "osapi.h"
#include "mqtt.h"
#include "config.h"
#include "debug.h"
#include "mem.h"
#include "user_mqtt.h"
#include "config.h"
#include "user_ir.h"
#include "user_json.h"
#include "base64.h"

static bool isMqttConnected = false;
static uint8_t lwtData[] = {"{\"device_status\":{\"live\":\"false\"}}"};
static uint8_t lwtDataLive[] = {"{\"device_status\":{\"live\":\"true\"}}"};


int get_mqtt_lwt_topic(uint8_t *topic)
{
	os_bzero(topic,sizeof(topic));
	os_sprintf(topic,"home/%s/wifi/%s/status",sysCfg.deviceCfg.shMac,sysCfg.device_id);
	return os_strlen(topic);
}


int get_mqtt_ir_cmd_topic(uint8_t *topic)
{
	os_bzero(topic,sizeof(topic));
	os_sprintf(topic,"home/%s/wifi/%s/cmd",sysCfg.deviceCfg.shMac,sysCfg.device_id);
	return os_strlen(topic);
}

int get_mqtt_ir_fb_topic(uint8_t *topic)
{
	os_bzero(topic,sizeof(topic));
	os_sprintf(topic,"home/%s/wifi/%s/fb",sysCfg.deviceCfg.shMac,sysCfg.device_id);
	return os_strlen(topic);
}

int get_mqtt_ir_learned_data_topic(uint8_t *topic)
{
	os_bzero(topic,sizeof(topic));
	os_sprintf(topic,"sh/%s/wifi/%s/learnedData",sysCfg.deviceCfg.shMac,sysCfg.device_id);
	return os_strlen(topic);
}



bool getMqttStatus(){
	return isMqttConnected;
}
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	isMqttConnected = true;
	INFO("MQTT: Connected\r\n");
	char topic[100];
	get_mqtt_lwt_topic(topic);
	MQTT_Subscribe(client, topic, 0);
	MQTT_Publish(client, topic, lwtDataLive, os_strlen(lwtDataLive), 2, 1);
	get_mqtt_ir_cmd_topic(topic);
	MQTT_Subscribe(client, topic, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	isMqttConnected = false;
	uint8_t topic[100];
	get_mqtt_lwt_topic(topic);
	MQTT_Publish(client, topic, lwtData, os_strlen(lwtData), 2, 1);
	INFO("MQTT: Disconnected\r\n");
	MQTT_Connect(client);//reconnect
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}




void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);

	char statusTopic[100];
	int statusTopiclen = get_mqtt_lwt_topic(statusTopic);

	char cmdTopic[100];
	int cmdTopicLen = get_mqtt_ir_cmd_topic(cmdTopic);

	if(os_strncmp(topicBuf,statusTopic,statusTopiclen) == 0){
		MQTT_UnSubscribe(client,statusTopic);
		MQTT_Publish(client, statusTopic, lwtDataLive, os_strlen(lwtDataLive), 2, 1);
	}else if(os_strncmp(topicBuf,cmdTopic,cmdTopicLen) == 0){
		user_ir_json_parse(dataBuf);

		/*char *pbuf = NULL;
		pbuf = (char *) os_zalloc(jsonSize);
		user_ir_json_create(pbuf);
		get_mqtt_ir_fb_topic(cmdTopic);
		MQTT_Publish(client, cmdTopic, pbuf, os_strlen(pbuf), 2, 0);
		os_free(pbuf);*/
	}

	os_free(topicBuf);
	os_free(dataBuf);
	os_printf("\r\n-----------------------\r\nfree Heap %d\r\n \
				   -----------------------\r\n",system_get_free_heap_size());
}


void user_init_mqtt(MQTT_Client *mqttClient){
	isMqttConnected = false;
	MQTT_InitConnection(mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);
	MQTT_InitClient(mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, 60/*sysCfg.mqtt_keepalive*/, 1);
	uint8_t topic[50];
	get_mqtt_lwt_topic(topic);
	MQTT_InitLWT(mqttClient, topic, lwtData, 2, 1);
	MQTT_OnConnected(mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(mqttClient, mqttPublishedCb);
	MQTT_OnData(mqttClient, mqttDataCb);
}
