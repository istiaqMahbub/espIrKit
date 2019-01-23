/*
 * meter_proto.c
 *
 *  Created on: Oct 26, 2016
 *      Author: kanok
 */
#include "meter_proto.h"
#include "osapi.h"
#include "user_interface.h"
#include "meterInterface.h"
#include <mem.h>


void ICACHE_FLASH_ATTR
get_time_stamp(struct time_stamp_t *now) {
	meterReadings_t meterReadings = getMeterReadings();
	now->YYYY = 2000 + (meterReadings.dateReading / 10000);
	now->MN = ((meterReadings.dateReading / 100) % 100);
	now->DD = (meterReadings.dateReading % 100);
	now->HH = (meterReadings.timeReading / 10000);
	now->MM = ((meterReadings.timeReading / 100) % 100);
	now->SS = (meterReadings.timeReading % 100);
}

void ICACHE_FLASH_ATTR
get_unreg_payload(uint8_t *payload_array) {
	meterReadings_t meterReadings = getMeterReadings();
	struct unreged_t *payload = (struct unreged_t *) os_zalloc(
			sizeof(struct unreged_t));
	payload->topic_type = UNREGED;
	payload->meter_type = 1;
	os_memcpy(payload->meter_id,meterReadings.meterIDReading,6);
	payload->no_of_bytes = sizeof(struct unreged_t);
	get_time_stamp(&payload->time_stamp);
	uint8_t *array = (uint8_t *) payload;
	os_memcpy(payload_array, array, sizeof(struct unreged_t));
	os_free(payload);
}

void ICACHE_FLASH_ATTR
get_electrical_payload(uint8_t *payload_array) {
	meterReadings_t meterReadings = getMeterReadings();
	struct electrical_t *payload = (struct electrical_t *) os_zalloc(
			sizeof(struct electrical_t));
	payload->topic_type = ELECTRICAL;
	payload->no_of_bytes = sizeof(struct electrical_t);
	os_memcpy(payload->meter_id,meterReadings.meterIDReading,6);
	get_time_stamp(&payload->time_stamp);
	payload->voltage = meterReadings.voltageReading;
	payload->current = meterReadings.currentReading;
	payload->sanction_demand = meterReadings.loadThresholdReading;
	payload->current_active_power = meterReadings.activePowerReading;
	payload->max_demand_of_current_month = meterReadings.maxDemandReading;
	payload->accumulated_active_consumption = meterReadings.accActiveConsumptionReading;
	payload->meter_constant = meterReadings.meterConstantReading;
	uint8_t *array = (uint8_t *) payload;
	os_memcpy(payload_array, array, sizeof(struct electrical_t));
	os_free(payload);
}

void ICACHE_FLASH_ATTR
get_billing_payload(uint8_t *payload_array) {
	meterReadings_t meterReadings = getMeterReadings();
	struct billing_t *payload = (struct billing_t *) os_zalloc(
			sizeof(struct billing_t));
	payload->topic_type = BILLING;
	payload->no_of_bytes = sizeof(struct billing_t);
	os_memcpy(payload->meter_id,meterReadings.meterIDReading,6);
	get_time_stamp(&payload->time_stamp);
	payload->accumulated_purchase_credit = meterReadings.accPurchasedCreditReading;
	payload->remaining_credit = meterReadings.remainingCreditReading;
	payload->credit_used_in_current_month = meterReadings.creditUsedInCurrentMonthReading;
	payload->low_credit_alert_value = meterReadings.lowCreditAlertValueReading;
	payload->emergency_credit_limit = meterReadings.overdraftCreditLimitReading;
	payload->tarrif_index = meterReadings.tariffIndexReading;
	payload->sequence_number = meterReadings.sequenceNumberReading;
	payload->key_no = meterReadings.keyNumberReading;
	uint8_t *array = (uint8_t *) payload;
	os_memcpy(payload_array, array, sizeof(struct billing_t));
	os_free(payload);
}

void ICACHE_FLASH_ATTR
get_token_response_payload(uint8_t *payload_array) {
	meterReadings_t meterReadings = getMeterReadings();
	struct token_response_t *payload = (struct token_response_t *) os_zalloc(
			sizeof(struct token_response_t));
	payload->topic_type = TOKEN_RESPONSE;
	payload->no_of_bytes = sizeof(struct token_response_t);
	os_memcpy(payload->meter_id,meterReadings.meterIDReading,6);
	get_time_stamp(&payload->time_stamp);
	payload->token_acceptance_flag = meterReadings.tokenResponseReading;
	os_printf("\r\nToken Response %d----------------------\r\n",payload->token_acceptance_flag);
	uint8_t *array = (uint8_t *) payload;
	os_memcpy(payload_array, array, sizeof(struct token_response_t));
	os_free(payload);
}




/*void ICACHE_FLASH_ATTR
test_serializer() {

	uint8_t *payload_array = (uint8_t *) os_zalloc(sizeof(struct unreged_t));
	get_unreg_payload(payload_array);

	uint8_t i = 0;
	os_printf("\r\nrecovered array(unreged_t): ");
	for (i = 0; i < sizeof(struct unreged_t); i++) {
		os_printf("%02x ", payload_array[i]);
	}
	struct unreged_t *unreg_payload = (struct unreged_t *) payload_array;
	os_printf(
			"\r\nrecovered unreged_t struct data: type: %d, len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d\r\n",
			unreg_payload->meter_type, unreg_payload->no_of_bytes,
			unreg_payload->time_stamp.YYYY, unreg_payload->time_stamp.MN,
			unreg_payload->time_stamp.DD, unreg_payload->time_stamp.HH,
			unreg_payload->time_stamp.MM, unreg_payload->time_stamp.SS);
	os_free(payload_array);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	payload_array = (uint8_t *) os_zalloc(sizeof(struct electrical_t));
	get_electrical_payload(payload_array);

	os_printf("\r\nrecovered array(unreged_t): ");
	for (i = 0; i < sizeof(struct electrical_t); i++) {
		os_printf("%02x ", payload_array[i]);
	}
	struct electrical_t *electrical_payload =
			(struct electrical_t *) payload_array;
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
	os_free(payload_array);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	payload_array = (uint8_t *) os_zalloc(sizeof(struct billing_t));
	get_billing_payload(payload_array);

	os_printf("\r\nrecovered array(billing_t): ");
	for (i = 0; i < sizeof(struct billing_t); i++) {
		os_printf("%02x ", payload_array[i]);
	}
	struct billing_t *billing_payload = (struct billing_t *) payload_array;
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
	os_free(payload_array);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	payload_array = (uint8_t *) os_zalloc(sizeof(struct token_response_t));
	get_token_response_payload(payload_array);

	os_printf("\r\nrecovered array(token_response_t): ");
	for (i = 0; i < sizeof(struct token_response_t); i++) {
		os_printf("%02x ", payload_array[i]);
	}
	struct token_response_t *token_response_payload =
			(struct token_response_t *) payload_array;
	os_printf(
			"\r\nrecovered billing_t struct data: len: %d, Year: %d, Month: %d, date: %d, hour: %d, min: %d, sec: %d, token_acceptance_flag: %d\r\n",
			token_response_payload->no_of_bytes,
			token_response_payload->time_stamp.YYYY,
			token_response_payload->time_stamp.MN,
			token_response_payload->time_stamp.DD,
			token_response_payload->time_stamp.HH,
			token_response_payload->time_stamp.MM,
			token_response_payload->time_stamp.SS,
			token_response_payload->token_acceptance_flag);
	os_free(payload_array);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
}*/

