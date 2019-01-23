/*
 * mesh_demo.h
 *
 *  Created on: Oct 7, 2016
 *      Author: kanok
 */

#ifndef INCLUDE_MESH_DEMO_H_
#define INCLUDE_MESH_DEMO_H_

void knk_mesh_init(void);
void knk_mesh_deinit(void);
void esp_mesh_send_meter_packet(uint8_t *data);

#endif /* INCLUDE_MESH_DEMO_H_ */
