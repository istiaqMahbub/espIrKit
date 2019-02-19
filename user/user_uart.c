/*
 * user_uart.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Md. Istiaq Mahbub
 */
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "uart.h"
#include "mem.h"
#include "user_interface.h"
#include "config.h"
#include "user_uart.h"

static volatile uint8_t tx_buf[512];
static volatile uint16_t tx_buf_idx;
static volatile uint16_t tx_buf_len;
static volatile bool is_uart_sending;

static os_timer_t *uart_timer;
static void uart_timer_func(os_timer_t *timer);

user_uartDataCallback onUartDataAvailable = NULL;

static volatile bool startF = FALSE;
static volatile bool validF = FALSE;
static volatile bool timeOutF = FALSE;
static volatile uint8 rxPacket[UART_RX_PKT_LEN];
static volatile uint16 rxPacketIdx = 0;
static volatile uint16_t rxPacketLen = HEADER_LEN;
static volatile uint16_t rxTimeoutCntr = 0;

static volatile os_timer_t tx_timer;
static void tx_timeoutfunc(void *arg);
static void tx_timer_start();


void ICACHE_FLASH_ATTR
user_uart_init(user_uartDataCallback onUartDataCb){
	os_printf("\r\nUart initializing...\r\n");
	startF = FALSE;
	validF = FALSE;
	timeOutF = FALSE;
	os_bzero(rxPacket,UART_RX_PKT_LEN);
	rxPacketIdx = 0;
	rxPacketLen = HEADER_LEN;
	//////////////////////////////
	onUartDataAvailable = onUartDataCb;
	uart_timer = (os_timer_t *)os_zalloc(sizeof(os_timer_t));
	if (NULL == uart_timer) return;
	os_timer_disarm(uart_timer);
	os_timer_setfn(uart_timer, (os_timer_func_t *)uart_timer_func, uart_timer);
	os_timer_arm(uart_timer, 5, true);
}

void ICACHE_FLASH_ATTR
user_uart_disable(void){
	if (NULL == uart_timer) return;
		os_timer_disarm(uart_timer);
}


static void ICACHE_FLASH_ATTR
tx_timeoutfunc(void *arg) {

	if (is_uart_sending) {
		//os_printf("\r\nHAL: sending %d = %c(%d) ", tx_buf_idx,tx_buf[tx_buf_idx],tx_buf[tx_buf_idx]);
		//Softuart_Putchar(&myModem.gsm_uart, myModem.tx_buf[myModem.tx_buf_idx]);
		uart_tx_one_char(UART0, tx_buf[tx_buf_idx]);
		tx_buf_idx++;
		if (tx_buf_idx >= tx_buf_len) {
			os_timer_disarm(&tx_timer);
			is_uart_sending = false;
			tx_buf_idx = 0;
			os_printf("\r\nHAL: sending finished\r\n");
			uart_tx_one_char_no_wait(UART0, END_PKT_FLAG_ARD);
		}
	} else {
		os_printf("\r\nHAL: not ready for tcp TX\r\n");
		is_uart_sending = false;
		os_timer_disarm(&tx_timer);
	}
}

static void ICACHE_FLASH_ATTR
tx_timer_start() {
	os_timer_disarm(&tx_timer);

	//Setup timer
	os_timer_setfn(&tx_timer,
			(os_timer_func_t *) tx_timeoutfunc, NULL);
	//Arm the timer
	//&some_timer is the pointer
	//1000 is the fire time in ms
	//0 for once and 1 for repeating
	os_timer_arm(&tx_timer, 5, 1);
}


void ICACHE_FLASH_ATTR
user_uart_packet_send(uint8_t cmd, uint8_t cmd_id, uint8 *payload, uint16 length) {
	os_printf("\r\nHAL: about to sending %d bytes\r\n", length);
	if (!is_uart_sending) {
		os_printf("\r\nHAL: sending %d bytes\r\n", length);
		uint16 len = length+HEADER_LEN;
		uint8_t lenH = len>>8;
		uint8_t lenL = len & 0x00FF;
		os_printf("H: %d, L: %d\r\n",lenH,lenL);
		uart_tx_one_char_no_wait(UART0, START_PKT_FLAG_ARD);
		uart_tx_one_char_no_wait(UART0, lenH);//0:st_pkt, 1:len, 2:cmd, 3:cmd_id, 4:data of len long, 5:end_pkt
		uart_tx_one_char_no_wait(UART0, lenL);//0:st_pkt, 1:len, 2:cmd, 3:cmd_id, 4:data of len long, 5:end_pkt
		uart_tx_one_char_no_wait(UART0, cmd);//tx cmd is 3
		uart_tx_one_char_no_wait(UART0, cmd_id);
		if(length>0){
			tx_buf_idx = 0;
			tx_buf_len = length;
			os_memcpy(tx_buf, payload, length);
			is_uart_sending = true;
			tx_timer_start();
		}else{
			uart_tx_one_char_no_wait(UART0, END_PKT_FLAG_ARD);
		}

	} else {
		os_printf("\r\ndata_send: not possible\r\n");
		//may be HAL level buffering needed
	}
}


static void ICACHE_FLASH_ATTR
uart_timer_func(os_timer_t *timer){
	//os_printf("c\r\n");
	if(startF){
		rxTimeoutCntr++;
		if(rxTimeoutCntr>500){
			os_printf("uart rx packet timeout\r\n");
			startF = FALSE;
			rxTimeoutCntr = 0;
		}
	}
	if(uart0_Available()){
		char _rx[1];
		rx_buff_deq(_rx, 1);
		//os_printf("%d = %d \r\n",_rx[0],rxPacketIdx);
	    if(_rx[0] == START_PKT_FLAG && startF == FALSE)
	    {
	        os_printf("uart rx started\r\n");
	        rxPacketIdx = 0;
	        rxPacketLen = HEADER_LEN;
	        startF = TRUE;
	        validF = FALSE;
	        rxTimeoutCntr = 0;
	    }
	    if(startF == TRUE)
	    {
	    	rxTimeoutCntr = 0;
	        if(rxPacketIdx < rxPacketLen)
	        {
	            if(rxPacketIdx == 3)
	            {
	                rxPacketLen = (uint16_t)(rxPacket[1]<<8)|(uint16_t)(rxPacket[2]);
	                os_printf("Packet len %d\r\n",rxPacketLen);
	            }
	            rxPacket[rxPacketIdx] = _rx[0];
	            rxPacketIdx = rxPacketIdx+1;
	        }
	        if(rxPacketIdx == rxPacketLen)//per packet length(boundary check)
	        {
	            if(rxPacket[rxPacketLen-1] == END_PKT_FLAG)
	            {
	                //save data
	            	//os_printf("packet data len: %d\r\n",rxPacketLen);
	            	//os_printf("Data %s\r\n", &rxPacket[HEADER_LEN-1]);
	            	uint8_t cmd = rxPacket[3];
	            	uint8_t cmd_id = rxPacket[4];
	            	int len = rxPacketLen - HEADER_LEN;
	            	if(onUartDataAvailable!=NULL){
	            		onUartDataAvailable(cmd,cmd_id,(uint8_t*)&rxPacket[HEADER_LEN-1],len);
	            	}
	            	validF = TRUE;
	            }
	            else
	            {
	                //invalid packet
	                validF = FALSE;
	                os_printf("invalid Packet\r\n");
	            }
	            startF = FALSE;
	            timeOutF = FALSE;
	        }
	        else if(rxPacketIdx > UART_RX_PKT_LEN)//max packet length(bondary check)
	        {
	        	os_printf("bigger Packet\r\n");
	            startF = FALSE;
	            timeOutF = FALSE;
	            validF = FALSE;
	        }
	        else
	        {

	        }

	    }
	}

}


