/*
 * MeterInterface.c
 *
 *  Created on: Oct 31, 2016
 *  Author: APL-SadequrRahman
 */
/*
 * SDK Include
 */
#include "ets_sys.h"
#include "osapi.h"
#include "debug.h"
#include "user_interface.h"
#include "mem.h"
#include "os_type.h"
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
/*
 * User Modules
 */
#include "meterInterface.h"
/*
 * Private definition
 */
#define CHECK_SUM_OFFSET		0x08
#define SEND_CMD_SIZE			18
#define SEND_TOKEN_SIZE			28
#define TxEn_GPIO 15
#define TxEn_GPIO_MUX PERIPHS_IO_MUX_MTDO_U
#define TxEn_GPIO_FUNC FUNC_GPIO15
#define RcvTaskDelay			(10)
#define Rec_TimeOut				50
/*
 * Private Data
 */
LOCAL uint8_t FixedHead[] = { 0xFE, 0xFE, 0xFE, 0xFE, 0x68, 0x99, 0x99, 0x99,
		0x99, 0x99, 0x99, 0x68, 0x01, 0x02 };
LOCAL uint8_t FixedHeadTokenPush5Byte[] = { 0xFE, 0xFE, 0xFE, 0xFE, 0x68 };
LOCAL uint8_t FixedHeadTokenPush3Byte[] = { 0x68, 0x00, 0x0C };
LOCAL uint8_t stopByte = 0x16;
LOCAL uint8_t startByte = 0x68;
LOCAL uint8_t gToken[10];
LOCAL bool gMeterIdvalid = false;
LOCAL uint8_t gMeterId[6];
LOCAL os_timer_t MeterRecTask;
LOCAL uint8_t *validData;
LOCAL mresponse_t *res;
LOCAL state_t state;
LOCAL uint8_t meterID[6];
LOCAL uint8_t byteCount = 0;
LOCAL uint8_t dataLength = 0;
LOCAL uint16_t rcvedChksum = 0;
LOCAL bool isChecksumOk = false;
LOCAL meter_t aplMeter;
LOCAL uint16_t lastCmd;
LOCAL CommandPoolType cmdPool;
LOCAL CommandPoolType lastCmdPool;
LOCAL unsigned int cmdPoolIdx;
LOCAL bool isCurrentCmdPoolFinished;
LOCAL uint8_t cmdRetryCnt;
LOCAL meterReadings_t meterReadings;
LOCAL bool MeterSendCmd(uint16_t _cmd, bool isToken);
LOCAL void meterRec(mresponse_t *arg);
LOCAL meterDataVariant_t parseData(mresponse_t *arg);
LOCAL volatile os_timer_t meterInterface_get_data_timer;
LOCAL void meterInterface_get_data_timeout_func(void *arg);
LOCAL uint16_t getNextCmd();
LOCAL uint8_t equivalentDec(uint8_t hex);

inline void setTxEnable(uint8_t val) {
	GPIO_OUTPUT_SET(TxEn_GPIO, val);
}

/*
 * Private Function Implementation
 */
LOCAL uint8_t ICACHE_FLASH_ATTR
calCheckSum(uint8_t *ptr, uint8_t len) {
	uint16_t checksum = CHECK_SUM_OFFSET;
	uint8_t i = 0;
	for (i = 0; i < len; i++) {
		checksum = checksum + *(ptr);
		ptr++;
	}

	return (checksum & 0x00FF);
}

LOCAL void ICACHE_FLASH_ATTR
MeterRecTaskCallBack(void *arg) {
	uint8_t d_temp;
	if (Softuart_Available(&aplMeter.meter_uart)) {
		d_temp = Softuart_Read(&aplMeter.meter_uart);
		//os_printf("Byte-> %d\r\n", d_temp);
		if (aplMeter.is_pending) {
			switch (state) {
			case packetStartCheck:
				if (d_temp == startByte) {
					rcvedChksum = d_temp;
					state = readingMeterID;
					byteCount++;
					//os_printf("Start Byte-> %d\r\n", d_temp);
				}
				break;
			case readingMeterID:
				rcvedChksum += d_temp;
				meterID[byteCount - 1] = d_temp;
				byteCount++;
				if (byteCount > 6) {
					state = readingFixedBytes;
					//os_printf("MeterID -> %s\r\n", meterID);
				}
				break;
			case readingFixedBytes:
				rcvedChksum += d_temp;
				byteCount++;
				//os_printf("Fixed Bytes-> %d\r\n", d_temp);
				//os_printf("Byte Count-> %d\r\n", byteCount);
				if (byteCount > 8) {
					state = extractData;
				}
				break;
			case extractData:
				rcvedChksum += d_temp;
				dataLength = d_temp;
				//os_printf("Valid Data Length-> %d\r\n", d_temp);
				validData = (uint8_t*) os_zalloc(dataLength);
				if (Softuart_Available(&aplMeter.meter_uart) >= dataLength) {
					uint8_t i = 0;
					for (i = 0; i < dataLength; i++) {
						validData[i] = Softuart_Read(&aplMeter.meter_uart);
						rcvedChksum += validData[i];
						//os_printf("valid Bytes-> %d\r\n", validData[i]);
					}
					//os_printf("Valid data Received\r\n");
					res = (mresponse_t*) os_zalloc(sizeof(mresponse_t));
					res->dataLen = dataLength;
					os_memcpy(res->meterID, meterID, sizeof(meterID));
					res->validData = validData;

				}
				byteCount += (dataLength + 1);
				state = checkingChksum;
				break;
			case checkingChksum:
				//os_printf("checkSum-> %d\r\n", d_temp);
				rcvedChksum = rcvedChksum & 0x00FF;
				if (rcvedChksum == d_temp) {
					isChecksumOk = true;
				} else {
					isChecksumOk = false;
					os_printf("checkSum-> rxed %d, calc %d\r\n", rcvedChksum,
							d_temp);
				}
				byteCount++;
				state = stopByteCheck;
				break;
			case stopByteCheck:
				//os_printf("Stop Byte-> %d\r\n", d_temp);
				byteCount++;
				//os_printf("Total Received Bytes-> %d\r\n", byteCount);
				if (d_temp == stopByte && isChecksumOk) {
					res->status = Okay;
					meterRec(res);
				}
				state = packetStartCheck;
				byteCount = 0;
				os_free(validData);
				os_free(res);
				break;
			}
		}
	}
}

LOCAL meterDataVariant_t ICACHE_FLASH_ATTR
parseData(mresponse_t *arg) {
	uint8_t usableDataLength = arg->dataLen;
	uint32_t fourByteParameter = 0;
	uint16_t twoByteParameter = 0;
	uint8_t oneByteParameter = 0;
	int32_t remainingCredit = 0;
	if (usableDataLength == 3) {
		if (lastCmd == tariffIndexReadingReq) {
			oneByteParameter = (arg->validData[2] - 0x33);
		} else {
			oneByteParameter = equivalentDec(arg->validData[2]);
		}
	} // If usable data length is 3
	else if (usableDataLength == 4) {
		if (lastCmd == frequencyReadingReq) {
			uint8_t X = equivalentDec(arg->validData[2]);
			uint8_t Y = equivalentDec(arg->validData[3]);
			twoByteParameter = ((uint16_t) Y * 100) + (uint16_t) X;
		} else {
			uint8_t X = (arg->validData[2] - 0x33);
			uint8_t Y = (arg->validData[3] - 0x33);
			twoByteParameter = (((uint16_t) Y & 0xFFFF) << 8)
					| ((uint16_t) X & 0xFF);
		}
	} // If usable data length is 4
	else if (usableDataLength == 5) {
		uint8_t X = equivalentDec(arg->validData[2]);
		uint8_t Y = equivalentDec(arg->validData[3]);
		uint8_t Z = equivalentDec(arg->validData[4]);
		twoByteParameter = (10000 * Z) + (Y * 100) + X;
		if (lastCmd == timeReadingReq) {
			fourByteParameter = (10000 * (uint32_t) Z) + ((uint32_t) Y * 100)
					+ (uint32_t) X;
		}
	} // If usable data length is 5
	else if (usableDataLength == 6) {
		if (lastCmd == accPurchasedCreditReadingReq
				|| lastCmd == lowCreditAlertValueReadingReq
				|| lastCmd == overdraftCreditLimitReadingReq
				|| lastCmd == remainingCreditReadingReq
				|| lastCmd == creditUsedInCurrentMonthReadingReq) {
			uint8_t W = arg->validData[2] - 0x33;
			uint8_t X = arg->validData[3] - 0x33;
			uint8_t Y = arg->validData[4] - 0x33;
			uint8_t Z = arg->validData[5] - 0x33;
			fourByteParameter = (((uint32_t) Z & 0xFFFFFFFF) << 24)
					| (((uint32_t) Y & 0xFFFFFF) << 16) | ((uint32_t) X << 8)
					| (uint32_t) W;
			remainingCredit = fourByteParameter;
			if (arg->validData[5] <= 0x32 || arg->validData[4] <= 0x32
					|| arg->validData[3] <= 0x32) {
				remainingCredit = ~(remainingCredit);
				remainingCredit = (remainingCredit) + 1;
				remainingCredit = ((remainingCredit) * (-1));
			}
		} // If above parameters are required
		else if (lastCmd == relayStatusReadingReq) {
			uint8_t W = equivalentDec(arg->validData[2]);
			uint8_t X = equivalentDec(arg->validData[3]);
			uint8_t Y = equivalentDec(arg->validData[4]);
			uint8_t Z = equivalentDec(arg->validData[5]);
			fourByteParameter = W;
		} else {
			uint8_t W = equivalentDec(arg->validData[2]);
			uint8_t X = equivalentDec(arg->validData[3]);
			uint8_t Y = equivalentDec(arg->validData[4]);
			uint8_t Z = equivalentDec(arg->validData[5]);
			//CalculatedValue = ((Z&0xFFFFFFFF)<<24)|((Y&0xFFFFFF)<<16)|(X<<8)|W;
			if (lastCmd == dateReadingReq) {
				fourByteParameter = ((uint32_t) Z * 10000)
						+ ((uint32_t) Y * 100) + (uint32_t) X;
			} else {
				fourByteParameter = ((uint32_t) Z * 1000000)
						+ ((uint32_t) Y * 10000) + ((uint32_t) X * 100)
						+ (uint32_t) W;
			}
		} //  else for other parameters

	} // if usable data length is 6

	meterDataVariant_t meterData;
	meterData.fourByteParameter = fourByteParameter;
	meterData.twoByteParameter = twoByteParameter;
	meterData.oneByteParameter = oneByteParameter;
	meterData.remainingCredit = remainingCredit;
	return meterData;
}

LOCAL void ICACHE_FLASH_ATTR
meterRec(mresponse_t *arg) {
	meterDataVariant_t meterData;
	if (lastCmd != meterIDReadingReq) {
		meterData = parseData(arg);
	}
	switch (lastCmd) {
	case tokenWriteReq: //TODO: exceptional case
		isCurrentCmdPoolFinished = true;
		lastCmdPool = POOL_TOKEN_RESP_TYPE;
		cmdPoolIdx = 0;
		meterReadings.tokenResponseReading = meterData.oneByteParameter;
		os_printf("\r\n TokenWrite Response %d\r\n",
				meterReadings.tokenResponseReading);
		break;
	case meterIDReadingReq:
		os_printf("\r\n meterIDReadingReq Response \r\n");
		int i, j;
		for (i = 0, j = 5; i < 6; i++, j--) {
			meterReadings.meterIDReading[j] = arg->validData[i + 2] - 0x33;
			gMeterId[i] = meterReadings.meterIDReading[j];
			os_printf("%d ", meterReadings.meterIDReading[j]);
		}
		gMeterIdvalid = true;
		user_mqtt_subscribe_by_non_string_topic(meterReadings.meterIDReading,
				0);
		break;
	case timeReadingReq:
		meterReadings.timeReading = meterData.fourByteParameter;
		os_printf("\r\n timeReadingReq Response %d\r\n",
				meterReadings.timeReading);
		break;
	case dateReadingReq:
		meterReadings.dateReading = meterData.fourByteParameter;
		os_printf("\r\n dateReadingReq Response %d\r\n",
				meterReadings.dateReading);
		break;
	case voltageReadingReq:
		meterReadings.voltageReading = meterData.twoByteParameter;
		os_printf("\r\n voltageReadingReq Response %d\r\n",
				meterReadings.voltageReading);
		break;
	case currentReadingReq:
		meterReadings.currentReading = meterData.twoByteParameter;
		os_printf("\r\n currentReadingReq Response %d\r\n",
				meterReadings.currentReading);
		break;
	case loadThresholdReadingReq:
		meterReadings.loadThresholdReading = meterData.twoByteParameter;
		os_printf("\r\n loadThresholdReadingReq Response %d\r\n",
				meterReadings.loadThresholdReading);
		break;
	case activePowerReadingReq:
		meterReadings.activePowerReading = meterData.twoByteParameter;
		os_printf("\r\n activePowerReadingReq Response %d\r\n",
				meterReadings.activePowerReading);
		break;
	case maxDemandReadingReq:
		meterReadings.maxDemandReading = meterData.twoByteParameter;
		os_printf("\r\n maxDemandReadingReq Response %d\r\n",
				meterReadings.maxDemandReading);
		break;
	case accActiveConsumptionReadingReq:
		meterReadings.accActiveConsumptionReading = meterData.fourByteParameter;
		os_printf("\r\n accActiveConsumptionReadingReq Response %d\r\n",
				meterReadings.accActiveConsumptionReading);
		break;
	case meterConstantReadingReq:
		meterReadings.meterConstantReading = meterData.fourByteParameter;
		os_printf("\r\n meterConstantReadingReq Response %d\r\n",
				meterReadings.meterConstantReading);
		break;
	case accPurchasedCreditReadingReq:
		meterReadings.accPurchasedCreditReading = meterData.fourByteParameter;
		os_printf("\r\n accPurchasedCreditReadingReq Response %d\r\n",
				meterReadings.accPurchasedCreditReading);
		break;
	case remainingCreditReadingReq:
		meterReadings.remainingCreditReading = meterData.remainingCredit;
		os_printf("\r\n remainingCreditReadingReq Response %d\r\n",
				meterReadings.remainingCreditReading);
		break;
	case creditUsedInCurrentMonthReadingReq:
		meterReadings.creditUsedInCurrentMonthReading =
				meterData.fourByteParameter;
		os_printf("\r\n creditUsedInCurrentMonthReadingReq Response %d\r\n",
				meterReadings.creditUsedInCurrentMonthReading);
		break;
	case lowCreditAlertValueReadingReq:
		meterReadings.lowCreditAlertValueReading = meterData.fourByteParameter;
		os_printf("\r\n lowCreditAlertValueReadingReq Response %d\r\n",
				meterReadings.lowCreditAlertValueReading);
		break;
	case overdraftCreditLimitReadingReq:
		meterReadings.overdraftCreditLimitReading = meterData.fourByteParameter;
		os_printf("\r\n overdraftCreditLimitReadingReq Response %d\r\n",
				meterReadings.overdraftCreditLimitReading);
		break;
	case tariffIndexReadingReq:
		meterReadings.tariffIndexReading = meterData.oneByteParameter;
		os_printf("\r\n tariffIndexReadingReq Response %d\r\n",
				meterReadings.tariffIndexReading);
		break;
	case openCoverEventReadingReq:
		os_printf("\r\n openCoverEventReadingReq Response\r\n");
		break;
		/*case currentUnbalanceEventReadingReq:
		 os_printf("\r\n currentUnbalanceEventReadingReq Response\r\n");
		 lastCmd = reverseEventReadingReq;
		 break;
		 case reverseEventReadingReq:
		 os_printf("\r\n reverseEventReadingReq Response\r\n");
		 lastCmd = overLoadEventReadingReq;
		 break;
		 case overLoadEventReadingReq:
		 os_printf("\r\n overLoadEventReadingReq Response\r\n");
		 lastCmd = overCurrentEventReadingReq;
		 break;
		 case overCurrentEventReadingReq:
		 os_printf("\r\n overCurrentEventReadingReq Response\r\n");
		 lastCmd = lowVoltageEventReadingReq;
		 break;
		 case lowVoltageEventReadingReq:
		 os_printf("\r\n lowVoltageEventReadingReq Response\r\n");
		 lastCmd = highVoltageEventReadingReq;
		 break;
		 case highVoltageEventReadingReq:
		 os_printf("\r\n highVoltageEventReadingReq Response\r\n");
		 lastCmd = meterIDReadingReq;
		 break;*/
	}
	aplMeter.is_pending = false;
	if (!isCurrentCmdPoolFinished) {
		lastCmd = getNextCmd();
		MeterSendCmd(lastCmd, false);
	} else {
		uint8_t *payload_array = NULL;
		uint8_t size = 0;
		switch (lastCmdPool) {
		case POOL_UNREG_TYPE:
			payload_array = (uint8_t *) os_zalloc(sizeof(struct unreged_t));
			size = sizeof(struct unreged_t);
			get_unreg_payload(payload_array);
			user_mqtt_publish_by_non_string_msg("E/1/UNREG", payload_array,
					size);
			break;
		case POOL_ELEC_TYPE:
			payload_array = (uint8_t *) os_zalloc(sizeof(struct electrical_t));
			size = sizeof(struct electrical_t);
			get_electrical_payload(payload_array);
			user_mqtt_publish_by_non_string_msg("E/1/ELEC", payload_array,
					size);
			break;
		case POOL_BILLING_TYPE:
			payload_array = (uint8_t *) os_zalloc(sizeof(struct billing_t));
			size = sizeof(struct billing_t);
			get_billing_payload(payload_array);
			user_mqtt_publish_by_non_string_msg("E/1/BILLING", payload_array,
					size);
			break;
		case POOL_TOKEN_RESP_TYPE:
			payload_array =
					(uint8_t *) os_zalloc(sizeof(struct token_response_t));
			size = sizeof(struct token_response_t);
			get_token_response_payload(payload_array);
			user_mqtt_publish_by_non_string_msg("E/1/TKNRSP", payload_array,
					size);
			break;
		}
		os_free(payload_array);
	}

}

LOCAL bool ICACHE_FLASH_ATTR
MeterSendCmd(uint16_t _cmd, bool isToken) {
	if (!aplMeter.is_pending) {
		uint8_t cmdSize = 0;
		uint8_t *pkt = NULL;
		if (isToken) {
			if (gMeterIdvalid){
				cmdSize = SEND_TOKEN_SIZE;
				pkt = (uint8_t*) os_zalloc(cmdSize);
				uint8_t tokIdx = 0;

				uint8_t cpyLen = sizeof(FixedHeadTokenPush5Byte);
				os_memcpy(pkt, FixedHeadTokenPush5Byte, cpyLen);
				tokIdx = cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

				cpyLen = 6;
				os_memcpy(&pkt[tokIdx], gMeterId, cpyLen);
				tokIdx += cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

				cpyLen = sizeof(FixedHeadTokenPush3Byte);
				os_memcpy(&pkt[tokIdx], FixedHeadTokenPush3Byte, cpyLen);
				tokIdx += cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

				cpyLen = 1;
				pkt[tokIdx] = (_cmd >> 8) & 0xFF;
				tokIdx += cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

				cpyLen = 1;
				pkt[tokIdx] = (_cmd & 0xFF);
				tokIdx += cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

				cpyLen = sizeof(gToken);
				uint8_t i, j;
				j =  cpyLen - 1;
				uint8_t tokStart = tokIdx;
				for (i = tokStart; i<(tokStart+cpyLen); i++, j--) {
					os_printf("\r\ni = %d, j= %d\r\n",i,j);
					pkt[i] = gToken[j] + 0x33;
				}
				tokIdx += cpyLen;
				os_printf("\r\ntokIdx %d\r\n",tokIdx);

			} else {
				os_printf("\r\nMeterId not Valid yet\r\n");
				return false;
			}
		} else {
			cmdSize = SEND_CMD_SIZE;
			pkt = (uint8_t*) os_zalloc(cmdSize);
			os_memcpy(pkt, FixedHead, sizeof(FixedHead));
			pkt[sizeof(FixedHead)] = (_cmd >> 8) & 0xFF;
			pkt[sizeof(FixedHead) + 1] = (_cmd & 0xFF);
		}
		pkt[cmdSize - 2] = calCheckSum(pkt, cmdSize - 2);
		pkt[cmdSize - 1] = stopByte;
		os_printf("Calc Checksum = %02x\r\n", pkt[cmdSize - 2]);
		uint8_t k;
		for (k = 0; k < cmdSize; k++) {
			os_printf("%02x ", pkt[k]);
		}
		os_memcpy(aplMeter.tx_buf, pkt, cmdSize);
		isChecksumOk = false;
		aplMeter.is_pending = true;
		aplMeter.tx_buf_idx = 0;
		aplMeter.tx_buf_len = cmdSize;
		//////////////////////////////////////////////////////
		os_timer_disarm(&meterInterface_get_data_timer);
		if (!isToken) {
			if (isCurrentCmdPoolFinished) {
				os_timer_arm(&meterInterface_get_data_timer, 60000, 0);

			} else {
				os_timer_arm(&meterInterface_get_data_timer, 5000, 0);
			}
		} else {
			lastCmd = tokenWriteReq;
			os_timer_arm(&meterInterface_get_data_timer, 5000, 0);
		}os_free(pkt);

		///////////////////////////////////////////////////////
		uint8 i = 0;
		setTxEnable(1);
		os_delay_us(10000);
		for (i = 0; i < cmdSize - 1; i++) {
			Softuart_Putchar(&aplMeter.meter_uart, aplMeter.tx_buf[i], false);
		}
		Softuart_Putchar(&aplMeter.meter_uart, aplMeter.tx_buf[i], true);
		os_printf("\r\nkHAL: sending finished\r\n");
		return true;
	} else {
		os_printf("\r\nModBUS already sending\r\n");
		return false;
	}
}

LOCAL void ICACHE_FLASH_ATTR
meterInterface_get_data_timeout_func(void *arg) {
	if (aplMeter.is_pending) { //retry last command
		os_printf("Rec TimeOut\r\n");
		cmdRetryCnt++;
		aplMeter.is_pending = false;
		if (state >= extractData) {
			os_free(validData);
			os_free(res);
		}
		state = packetStartCheck;
		if (cmdRetryCnt < 3) {
			if (lastCmd == tokenWriteReq) {
				MeterSendCmd(lastCmd, true);
			} else {
				MeterSendCmd(lastCmd, false);
			}
		} else { //start from the first command
			MeterInterface_init();
		}
	} else { //start from the first command
		lastCmd = getNextCmd();
		MeterSendCmd(lastCmd, false);
	}
}

LOCAL uint16_t ICACHE_FLASH_ATTR
getNextCmd() {
	uint16_t nextCmd = 0;
	switch (cmdPool) {
	case POOL_UNREG_TYPE:
		nextCmd = unregCmdPool[cmdPoolIdx];
		cmdPoolIdx++;
		if (cmdPoolIdx >= NELEMS(unregCmdPool))
		{
			cmdPoolIdx = 0;
			lastCmdPool = cmdPool;
			cmdPool = POOL_ELEC_TYPE;
			isCurrentCmdPoolFinished = true;
		} else {
			isCurrentCmdPoolFinished = false;
		}
		break;
	case POOL_ELEC_TYPE:
		nextCmd = electricalCmdPool[cmdPoolIdx];
		cmdPoolIdx++;
		if (cmdPoolIdx >= NELEMS(electricalCmdPool))
		{
			cmdPoolIdx = 0;
			lastCmdPool = cmdPool;
			cmdPool = POOL_BILLING_TYPE;
			isCurrentCmdPoolFinished = true;
		} else {
			isCurrentCmdPoolFinished = false;
		}
		break;
	case POOL_BILLING_TYPE:
		nextCmd = billingCmdPool[cmdPoolIdx];
		cmdPoolIdx++;
		if (cmdPoolIdx >= NELEMS(billingCmdPool))
		{
			cmdPoolIdx = 0;
			lastCmdPool = cmdPool;
			cmdPool = POOL_UNREG_TYPE;
			isCurrentCmdPoolFinished = true;
		} else {
			isCurrentCmdPoolFinished = false;
		}
		break;
	}
	return nextCmd;
}

LOCAL uint8_t ICACHE_FLASH_ATTR
equivalentDec(uint8_t hex) {
	return hex - (0x33 + ((hex - 0x33) / 16) * 6);
}

/*
 * Public Function Implementation
 */
void ICACHE_FLASH_ATTR
MeterInterface_init() {
	validData = NULL;
	res = NULL;
	state = packetStartCheck;
	Softuart_SetPinRx(&aplMeter.meter_uart, 5); //APL5//D5
	Softuart_SetPinTx(&aplMeter.meter_uart, 4); //APL4//D6
	//startup
	Softuart_EnableRs485(&aplMeter.meter_uart, 15);
	Softuart_Init(&aplMeter.meter_uart, 9600);
	PIN_FUNC_SELECT(TxEn_GPIO_MUX, TxEn_GPIO_FUNC);
	setTxEnable(0);
	aplMeter.is_pending = false;
	aplMeter.tx_buf_idx = 0;
	aplMeter.tx_buf_len = 0;

	os_timer_disarm(&MeterRecTask);
	os_timer_setfn(&MeterRecTask, (os_timer_func_t*) MeterRecTaskCallBack,
			(void*) 0);
	os_timer_arm(&MeterRecTask, RcvTaskDelay, 10);

	os_timer_disarm(&meterInterface_get_data_timer);
	//Setup timer
	os_timer_setfn(&meterInterface_get_data_timer,
			(os_timer_func_t *) meterInterface_get_data_timeout_func, NULL);
	//Arm the timer
	//&some_timer is the pointer
	//1000 is the fire time in ms
	gMeterIdvalid = false;
	cmdRetryCnt = 0;
	cmdPoolIdx = 0;
	isCurrentCmdPoolFinished = false;
	cmdPool = POOL_UNREG_TYPE;
	lastCmdPool = cmdPool;
	lastCmd = getNextCmd();
	MeterSendCmd(lastCmd, false);
}

bool ICACHE_FLASH_ATTR
MeterSendToken(uint8_t *token) {
	os_memcpy(gToken, token, 10);
	return MeterSendCmd(tokenWriteReq, true);
}

meterReadings_t ICACHE_FLASH_ATTR
getMeterReadings(void) {
	return meterReadings;
}
