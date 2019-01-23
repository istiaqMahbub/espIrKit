/*
 * meter_proto.h
 *
 *  Created on: Oct 26, 2016
 *      Author: kanok
 */

#ifndef METERINTERFACE_INCLUDE_METER_PROTO_H_
#define METERINTERFACE_INCLUDE_METER_PROTO_H_

//#include "mesh_parser.h"
#include "c_types.h"

#define UNREGED_PAYLOAD_SIZE 16+1
#define ELECTRICAL_PAYLOAD_SIZE 31+1
#define BILLING_PAYLOAD_SIZE 37+1
#define TOKEN_RESPONSE_PAYLOAD_SIZE 26+1


//NOTE: The attribute packed means that the compiler will not add padding between fields of the struct


typedef enum {
	//_Accept  = 0,  /**< Dont Know yet */
	_Reject = 1, /**< Token Response From Meter */
	_Old = 2, /**< Token Response From Meter */
	_Used = 3, /**< Token Response From Meter */
	_Ok = 8, /**< Token Response From Meter */
	//_Err_01  = 5,  /**< Dont Know yet */
	//_Err_02  = 6,  /**< Dont Know yet */
	//_Err_03  = 7,  /**< Dont Know yet */
	_Err_04 = 13, /**< Token Response From Meter */
	//_Err_05  = 9,  /**< Dont Know yet */
	//_Err_06  = 10, /**< Dont Know yet */
	//_Err_07  = 11, /**< Dont Know yet */
	//_Err_08  = 12, /**< Dont Know yet */
	_Pls_2nd = 7, /**< Token Response From Meter */
} tokenResponseFromMeter_t;

struct time_stamp_t {
	uint16_t YYYY;
	uint8_t MN;
	uint8_t DD;
	uint8_t HH;
	uint8_t MM;
	uint8_t SS;
}__packed;

struct unreged_t {
	uint8_t topic_type;
	uint8_t no_of_bytes;
	uint8_t meter_id[6];
	struct time_stamp_t time_stamp;
	uint8_t meter_type;
	uint8_t checksum;
}__packed;

struct electrical_t {
	uint8_t topic_type;
	uint8_t no_of_bytes;
	uint8_t meter_id[6];
	struct time_stamp_t time_stamp;
	uint16_t voltage;
	uint16_t current;
	uint16_t sanction_demand;
	uint16_t current_active_power;
	uint16_t max_demand_of_current_month;
	uint32_t accumulated_active_consumption;
	uint16_t meter_constant;
	uint8_t checksum;
}__packed;

struct billing_t {
	uint8_t topic_type;
	uint8_t no_of_bytes;
	uint8_t meter_id[6];
	struct time_stamp_t time_stamp;
	uint32_t accumulated_purchase_credit;
	uint32_t remaining_credit;
	uint32_t credit_used_in_current_month;
	uint16_t low_credit_alert_value;
	uint32_t emergency_credit_limit;
	uint8_t tarrif_index;
	uint8_t sequence_number;
	uint16_t key_no;
	uint8_t checksum;
}__packed;

struct token_response_t {
	uint8_t topic_type;
	uint8_t no_of_bytes;
	uint8_t meter_id[6];
	struct time_stamp_t time_stamp;
	uint8_t token_number[10];
	tokenResponseFromMeter_t token_acceptance_flag;
	uint8_t checksum;
}__packed;


struct token_push_msg_t {
	uint8_t no_of_bytes;
	uint8_t push_msg_type;
	uint8_t meter_id[6];
	uint8_t token_number[10];
	uint8_t checksum;
}__packed;


enum PUSH_MSG_TYPE {
	REG_MESG = 0, TOKEN_MSG,
};

enum TOPIC_ENUM {
	UNREGED = 0, ELECTRICAL, BILLING, TOKEN_RESPONSE,
};
enum METER_TYPE_ENUM {
	SINGLE_PHASE = 1, THREE_PHASE = 3,
};
/*enum TOKEN_ACCEPTANCE_FLAG {
	ACCEPTED = 0, REJECTED, USED,
};*/





#endif /* METERINTERFACE_INCLUDE_METER_PROTO_H_ */
