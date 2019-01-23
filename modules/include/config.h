/* config.h
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"


#define id6_1(id) (((uint8*)(id))[0])
#define id6_2(id) (((uint8*)(id))[1])
#define id6_3(id) (((uint8*)(id))[2])
#define id6_4(id) (((uint8*)(id))[3])
#define id6_5(id) (((uint8*)(id))[4])
#define id6_6(id)  (((uint8*)(id))[5])

#define ID6TOSTR(id) id6_1(id), \
    id6_2(id), \
    id6_3(id), \
    id6_4(id), \
    id6_5(id), \
    id6_6(id)

#define ID6STR "%02x:%02x:%02x:%02x:%02x:%02x"

typedef enum ComModes {
	MODE_SOFTAP = 0,
	MODE_STA,
	MODE_MESH
}DeviceComModes;

typedef struct{
	uint8_t mesh_group_id[MESH_GROUP_ID_LEN];//02:30:12:25:AA:B6 when string
	uint8_t mesh_ssid_prefix[16];
	uint8_t mesh_pass[32];
	uint8_t isMeshEnable;
}MeshConfig;

typedef struct{
	uint8_t server_host[64];
	uint32_t server_port;
}ServerConfig;


typedef struct{
	DeviceComModes comMode;
	uint8_t deviceType[10];
	uint8_t deviceName[32];
	uint8_t shMac[18];//02:30:12:25:AA:B6
}DeviceConfig;

typedef struct{
	uint32_t usedKwHr;
	uint8_t relayStatus;
}DeviceData;


typedef struct{
	uint32_t cfg_holder;
	uint8_t device_id[16];

	uint8_t sta_ssid[64];
	uint8_t sta_pwd[64];
	uint32_t sta_type;

	uint8_t mqtt_host[64];
	uint32_t mqtt_port;
	uint8_t mqtt_user[32];
	uint8_t mqtt_pass[32];
	uint32_t mqtt_keepalive;
	uint8_t security;
	MeshConfig meshCfg;
	ServerConfig serverCfg;
	uint8_t softAp_pass[32];
	DeviceConfig deviceCfg;
	DeviceData deviceData;
} SYSCFG;

typedef struct {
    uint8 flag;
    uint8 pad[3];
} SAVE_FLAG;

void ICACHE_FLASH_ATTR CFG_Save();
void ICACHE_FLASH_ATTR CFG_Load();
void ICACHE_FLASH_ATTR CFG_saveStaCfg(struct station_config sta_conf, bool save);
void ICACHE_FLASH_ATTR CFG_saveSoftApPass(uint8_t *pass, bool save);
void ICACHE_FLASH_ATTR CFG_saveMeshConfig(MeshConfig meshCfg, bool save);
void ICACHE_FLASH_ATTR CFG_saveGwConfig(ServerConfig serverCfg, bool save);
void ICACHE_FLASH_ATTR CFG_saveDeviceConfig(DeviceConfig deviceCfg, bool save);
void ICACHE_FLASH_ATTR CFG_saveDeviceData(DeviceData deviceData, bool save);
bool ICACHE_FLASH_ATTR str_mac_to_byte_mac(uint8_t *str_mac, uint8_t *byte_mac);
bool ICACHE_FLASH_ATTR str_ip_to_byte_ip(uint8_t *str_ip, uint8_t *byte_ip);
int ICACHE_FLASH_ATTR Atoi(uint8_t* str);

extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
