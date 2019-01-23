/******************************************************************************
 * Copyright 2015-2016 Espressif Systems
 *
 * FileName: mesh_none.c
 *
 * Description: mesh demo for mesh topology parser
 *
 * Modification history:
 *     2016/03/15, v1.1 create this file.
*******************************************************************************/
#include "mem.h"
#include "osapi.h"
#include "master_mesh_parser.h"
#include "meter_proto.h"
#include "user_interface.h"

void ICACHE_FLASH_ATTR
mesh_none_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len)
{
    MESH_PARSER_PRINT("%s\n", __func__);
    if(len>0){
    	switch(pdata[0]){
    	case UNREGED:
    		if(pdata[1] == UNREGED_PAYLOAD_SIZE) {
    			struct unreged_t *unreg_payload = (struct unreged_t *) pdata;
    			os_printf("\r\n\r\n$$$$$$$$$$$$$$src mac: " MACSTR "$$$$$$$$$$$$\r\n", MAC2STR(unreg_payload->meter_id));
    			os_printf(
    						"\r\nrecovered unreged_t struct data: type: %d, len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d\r\n",
    						unreg_payload->meter_type, unreg_payload->no_of_bytes,
    						unreg_payload->time_stamp.YYYY, unreg_payload->time_stamp.MN,
    						unreg_payload->time_stamp.DD, unreg_payload->time_stamp.HH,
    						unreg_payload->time_stamp.MM, unreg_payload->time_stamp.SS);
    			user_mqtt_publish_by_non_string_msg("E/1/UNREG",pdata,len);
    		}
    		break;
    	case ELECTRICAL:
    		if(pdata[1] == ELECTRICAL_PAYLOAD_SIZE) {
    			struct electrical_t *electrical_payload = (struct electrical_t *) pdata;
    			os_printf("\r\n\r\n$$$$$$$$$$$$$$src mac: " MACSTR "$$$$$$$$$$$$\r\n", MAC2STR(electrical_payload->meter_id));
    			os_printf(
    						"\r\nrecovered electrical_t struct data: len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d, voltage: %d, current: %d, sanction_demand: %d, current_active_power: %d, max_demand_of_current_month: %d, accumulated_active_consumption: %d, meter_constant: %d\r\n",
    						electrical_payload->no_of_bytes,
    						electrical_payload->time_stamp.YYYY,
    						electrical_payload->time_stamp.MN,
    						electrical_payload->time_stamp.DD,
    						electrical_payload->time_stamp.HH,
    						electrical_payload->time_stamp.MM,
    						electrical_payload->time_stamp.SS, electrical_payload->voltage,
    						electrical_payload->current, electrical_payload->sanction_demand,
    						electrical_payload->current_active_power,
    						electrical_payload->max_demand_of_current_month,
    						electrical_payload->accumulated_active_consumption,
    						electrical_payload->meter_constant);
    			user_mqtt_publish_by_non_string_msg("E/1/ELEC",pdata,len);
    		}
    		break;
    	case BILLING:
    		if(pdata[1] == BILLING_PAYLOAD_SIZE) {
    			struct billing_t *billing_payload = (struct billing_t *) pdata;
    			os_printf("\r\n\r\n$$$$$$$$$$$$$$src mac: " MACSTR "$$$$$$$$$$$$\r\n", MAC2STR(billing_payload->meter_id));
    			os_printf(
    						"\r\nrecovered billing_t struct data: len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d, accumulated_purchase_credit: %d, remaining_credit: %d, credit_used_in_current_month: %d, low_credit_alert_value: %d, emergency_credit_limit: %d, tarrif_index: %d, sequence_number: %d, key_no: %d\r\n",
    						billing_payload->no_of_bytes, billing_payload->time_stamp.YYYY,
    						billing_payload->time_stamp.MN, billing_payload->time_stamp.DD,
    						billing_payload->time_stamp.HH, billing_payload->time_stamp.MM,
    						billing_payload->time_stamp.SS,
    						billing_payload->accumulated_purchase_credit,
    						billing_payload->remaining_credit,
    						billing_payload->credit_used_in_current_month,
    						billing_payload->low_credit_alert_value,
    						billing_payload->emergency_credit_limit,
    						billing_payload->tarrif_index, billing_payload->sequence_number,
    						billing_payload->key_no);
    			user_mqtt_publish_by_non_string_msg("E/1/BILLING",pdata,len);
    		}
    		break;
    	case TOKEN_RESPONSE:
    		if(pdata[1] == TOKEN_RESPONSE_PAYLOAD_SIZE) {
    			struct token_response_t *token_response_payload = (struct token_response_t *) pdata;
    			os_printf("\r\n\r\n$$$$$$$$$$$$$$src mac: " MACSTR "$$$$$$$$$$$$\r\n", MAC2STR(token_response_payload->meter_id));
    			os_printf(
    						"\r\nrecovered token_response_t struct data: len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d, token_acceptance_flag: %d\r\n",
    						token_response_payload->no_of_bytes,
    						token_response_payload->time_stamp.YYYY,
    						token_response_payload->time_stamp.MN,
    						token_response_payload->time_stamp.DD,
    						token_response_payload->time_stamp.HH,
    						token_response_payload->time_stamp.MM,
    						token_response_payload->time_stamp.SS,
    						token_response_payload->token_acceptance_flag);
    			user_mqtt_publish_by_non_string_msg("E/1/TKNRSP",pdata,len);
    		}
    		break;
    	}
    }

}
