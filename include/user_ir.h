/*
 * user_ir.h
 *
 *  Created on: Aug 7, 2017
 *  Author: Istiaq Mahbub
 */

#ifndef USER_IR_H_
#define USER_IR_H_


typedef void (*user_irCmdCallback)(uint8_t cmd, uint8_t cmd_id, uint8_t *data, int len);

void user_ir_cmd_init(user_irCmdCallback onIrCmdCb);
void user_ir_json_create(char *pbuf,uint8_t cmd,uint8_t id_cmd,uint16_t len_data,uint8_t *data);
void user_ir_json_parse(uint8_t *data);


#endif /* USER_IR_H_ */
