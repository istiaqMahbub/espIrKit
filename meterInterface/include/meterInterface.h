/*
 * MeterInterface.h
 *
 *  Created on: Oct 31, 2016
 *      Author: APL-SadequrRahman
 */

#ifndef METERINTERFACE_INCLUDE_METERINTERFACE_H_
#define METERINTERFACE_INCLUDE_METERINTERFACE_H_

#include "softuart.h"
#include "meter_proto.h"

#define MeterTxPin		5
#define MeterRxPin		4

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))



typedef enum {
	meterIDReadingReq = 0x65F3,
	timeReadingReq = 0x44F3,
	dateReadingReq = 0x43F3,
	voltageReadingReq = 0x44E9,
	currentReadingReq = 0x54E9,
	loadThresholdReadingReq = 0x3E15,
	activePowerReadingReq = 0x63E9,
	maxDemandReadingReq = 0x43D3,
	accActiveConsumptionReadingReq = 0x43C3,
	meterConstantReadingReq = 0x63F3,
	accPurchasedCreditReadingReq = 0x5715,
	remainingCreditReadingReq = 0x5515,
	creditUsedInCurrentMonthReadingReq = 0xE315,
	lowCreditAlertValueReadingReq = 0x5215,
	overdraftCreditLimitReadingReq = 0x5115,
	tariffIndexReadingReq = 0x3A15,
	openCoverEventReadingReq = 0x3323,
	currentUnbalanceEventReadingReq = 0x3331,
	reverseEventReadingReq = 0x3327,
	overLoadEventReadingReq = 0x3324,
	overCurrentEventReadingReq = 0x3330,
	lowVoltageEventReadingReq = 0x332D,
	highVoltageEventReadingReq = 0x332C,
	acceptedCreditTokenReadingReq = 0x2B33,
	keyNumberReadingReq = 0x1595,
	sequenceNumberReadingReq = 0x15A5,
	powerFactorReadingReq = 0xE984,
	relayStatusReadingReq = 0x157F,
	frequencyReadingReq = 0xEB43,
	tokenWriteReq = 0x8614
} CommandType;

typedef enum {
	POOL_UNREG_TYPE, POOL_ELEC_TYPE, POOL_BILLING_TYPE, POOL_TOKEN_RESP_TYPE
} CommandPoolType;

typedef enum {
	packetStartCheck = 0,
	readingMeterID,
	readingFixedBytes,
	extractData,
	checkingChksum,
	stopByteCheck
} state_t;

typedef enum {
	Okay = 0, timeoutError,
} mstatus_t;

typedef struct {
	mstatus_t status;
	uint8_t meterID[6];
	uint8_t dataLen;
	uint8_t *validData;

} mresponse_t;

typedef struct {
	Softuart meter_uart;
	volatile is_pending;
	volatile char tx_buf[50];
	volatile unsigned int tx_buf_idx;
	volatile unsigned int tx_buf_len;
} meter_t;

typedef struct {
	uint32_t fourByteParameter;
	uint16_t twoByteParameter;
	uint8_t oneByteParameter;
	int32_t remainingCredit;
} meterDataVariant_t;


typedef struct {
	uint8_t meterIDReading[6];
	uint32_t timeReading;
	uint32_t dateReading;
	uint16_t voltageReading;
	uint16_t currentReading;
	uint16_t loadThresholdReading;
	uint16_t activePowerReading;
	uint16_t maxDemandReading;
	uint32_t accActiveConsumptionReading;
	uint32_t meterConstantReading;
	uint32_t accPurchasedCreditReading;
	int32_t remainingCreditReading;
	uint32_t creditUsedInCurrentMonthReading;
	uint32_t lowCreditAlertValueReading;
	uint32_t overdraftCreditLimitReading;
	uint8_t tariffIndexReading;
	/*openCoverEventReadingReq           =  0x3323,
	currentUnbalanceEventReadingReq    =  0x3331,
	reverseEventReadingReq             =  0x3327,
	overLoadEventReadingReq            =  0x3324,
	overCurrentEventReadingReq         =  0x3330,
	lowVoltageEventReadingReq          =  0x332D,
	highVoltageEventReadingReq         =  0x332C*/
	//acceptedCreditTokenReading;
	uint16_t keyNumberReading;
	uint8_t sequenceNumberReading;
	uint16_t powerFactorReading;
	uint8_t relayStatusReading;
	uint16_t frequencyReading;
	tokenResponseFromMeter_t tokenResponseReading;
}meterReadings_t;

static uint16 unregCmdPool[] = {
							meterIDReadingReq,\
							timeReadingReq,\
							dateReadingReq
						};
static electricalCmdPool[] = {
								meterIDReadingReq,\
								timeReadingReq,\
								dateReadingReq,\
								accPurchasedCreditReadingReq,\
								currentReadingReq,\
								loadThresholdReadingReq,\
								activePowerReadingReq,\
								maxDemandReadingReq,\
								accActiveConsumptionReadingReq,\
								meterConstantReadingReq
							};

static billingCmdPool[] = {
								meterIDReadingReq,\
								timeReadingReq,\
								dateReadingReq,\
								voltageReadingReq,\
								currentReadingReq,\
								loadThresholdReadingReq,\
								activePowerReadingReq,\
								maxDemandReadingReq,\
								accActiveConsumptionReadingReq,\
								meterConstantReadingReq,\
								remainingCreditReadingReq,\
								creditUsedInCurrentMonthReadingReq,\
								lowCreditAlertValueReadingReq,\
								overdraftCreditLimitReadingReq,\
								tariffIndexReadingReq,\
							};

typedef void (*meterReceivedCallBack)(mresponse_t *arg);

/*
 * Public Functions Declaration
 */
void MeterInterface_init(void);
bool MeterSendToken(uint8_t *token);
meterReadings_t getMeterReadings(void);

#endif /* METERINTERFACE_INCLUDE_METERINTERFACE_H_ */
