/*
 * tcp_server.h
 *
 *  Created on: Oct 2, 2016
 *      Author: kanok
 */

#ifndef TCPSERVER_INCLUDE_TCP_SERVER_H_
#define TCPSERVER_INCLUDE_TCP_SERVER_H_

#define SERVER_LOCAL_PORT   1112

bool mesh_create_send_packet(uint8_t *dst_address, uint8_t *data, uint8_t data_len);
#endif /* TCPSERVER_INCLUDE_TCP_SERVER_H_ */
