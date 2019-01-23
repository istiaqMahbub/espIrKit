/*
 * user_uart.h
 *
 *  Created on: Aug 7, 2017
 *  Author: Istiaq Mahbub
 */

#ifndef USER_UART_H_
#define USER_UART_H_

#define START_PKT_FLAG_ARD 100
#define END_PKT_FLAG_ARD   101

#define START_PKT_FLAG 200
#define END_PKT_FLAG   201
#define UART_TX_PKT_LEN SINGLE_DATA_BLOCK_SIZE_MAX+2+3
#define HEADER_LEN 6
#define UART_RX_PKT_LEN 512+HEADER_LEN

typedef void (*user_uartDataCallback)(uint8_t cmd, uint8_t cmd_id, uint8_t *data, int len);

void user_uart_disable(void);
void user_uart_init(user_uartDataCallback onUartDataCb);
void user_uart_packet_send(uint8_t cmd, uint8_t cmd_id, uint8 *payload, uint16 length);

#endif /* USER_UART_H_ */
