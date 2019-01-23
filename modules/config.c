/*
 /* config.c
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
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "config.h"
#include "user_config.h"


SYSCFG sysCfg;
SAVE_FLAG saveFlag;

void ICACHE_FLASH_ATTR
CFG_Save() {
	spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
	(uint32 *) &saveFlag, sizeof(SAVE_FLAG));

	if (saveFlag.flag == 0) {
		spi_flash_erase_sector(CFG_LOCATION + 1);
		spi_flash_write((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 1;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &saveFlag, sizeof(SAVE_FLAG));
	} else {
		spi_flash_erase_sector(CFG_LOCATION + 0);
		spi_flash_write((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &saveFlag, sizeof(SAVE_FLAG));
	}
}

void ICACHE_FLASH_ATTR
CFG_Load() {

	os_printf("\r\nload ...\r\n");
	spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
	(uint32 *) &saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		spi_flash_read((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &sysCfg, sizeof(SYSCFG));
	} else {
		spi_flash_read((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
		(uint32 *) &sysCfg, sizeof(SYSCFG));
	}
	if (sysCfg.cfg_holder != CFG_HOLDER) {
		os_memset(&sysCfg, 0x00, sizeof sysCfg);

		sysCfg.cfg_holder = CFG_HOLDER;
		os_bzero(sysCfg.sta_ssid,sizeof(sysCfg.sta_ssid));
		os_sprintf(sysCfg.sta_ssid, "%s", STA_SSID);
		os_bzero(sysCfg.sta_pwd,sizeof(sysCfg.sta_pwd));
		os_sprintf(sysCfg.sta_pwd, "%s", STA_PASS);
		sysCfg.sta_type = STA_TYPE;

		os_bzero(sysCfg.device_id,sizeof(sysCfg.device_id));
		os_sprintf(sysCfg.device_id, "%08X", system_get_chip_id());
		os_bzero(sysCfg.mqtt_host,sizeof(sysCfg.mqtt_host));
		os_sprintf(sysCfg.mqtt_host, "%s", MQTT_HOST);
		sysCfg.mqtt_port = MQTT_PORT;
		os_bzero(sysCfg.mqtt_user,sizeof(sysCfg.mqtt_user));
		os_sprintf(sysCfg.mqtt_user, "%s", MQTT_USER);
		os_bzero(sysCfg.mqtt_pass,sizeof(sysCfg.mqtt_pass));
		os_sprintf(sysCfg.mqtt_pass, "%s", MQTT_PASS);

		sysCfg.security = DEFAULT_SECURITY; /* default non ssl */

		sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;

		os_bzero(sysCfg.softAp_pass,sizeof(sysCfg.softAp_pass));
		os_sprintf(sysCfg.softAp_pass, "%s", SOFTAP_PASS);

		os_bzero(sysCfg.meshCfg.mesh_group_id,sizeof(sysCfg.meshCfg.mesh_group_id));
		os_memcpy(sysCfg.meshCfg.mesh_group_id, MESH_GROUP_ID,	MESH_GROUP_ID_LEN);
		os_bzero(sysCfg.meshCfg.mesh_ssid_prefix,sizeof(sysCfg.meshCfg.mesh_ssid_prefix));
		os_sprintf(sysCfg.meshCfg.mesh_ssid_prefix, SOFTAP_SSID, system_get_chip_id());
		os_bzero(sysCfg.meshCfg.mesh_pass,sizeof(sysCfg.meshCfg.mesh_pass));
		os_sprintf(sysCfg.meshCfg.mesh_pass, "%s", MESH_PASSWD);
		sysCfg.meshCfg.isMeshEnable = 0;
		os_bzero(sysCfg.serverCfg.server_host,sizeof(sysCfg.serverCfg.server_host));
		os_sprintf(sysCfg.serverCfg.server_host, "%s", MQTT_HOST);
		sysCfg.serverCfg.server_port = server_port;

		sysCfg.deviceCfg.comMode = MODE_STA;
		os_bzero(sysCfg.deviceCfg.deviceType,sizeof(sysCfg.deviceCfg.deviceName));
		os_sprintf(sysCfg.deviceCfg.deviceType, "SMWFLS-01");
		os_bzero(sysCfg.deviceCfg.deviceName,sizeof(sysCfg.deviceCfg.deviceName));
		os_sprintf(sysCfg.deviceCfg.deviceName, "APL WiFi Light Sensor");
		os_bzero(sysCfg.deviceCfg.shMac,sizeof(sysCfg.deviceCfg.shMac));
		os_sprintf(sysCfg.deviceCfg.shMac,"%s","02:30:12:25:AA:B6");
		os_printf(" default configuration\r\n");

		CFG_Save();
	}
	os_printf("\r\nSoftApPass(knk): %s\r\n", sysCfg.softAp_pass);
	os_printf("\r\nMeshSSID(knk): %s\r\n", sysCfg.meshCfg.mesh_ssid_prefix);
	os_printf("\r\nMeshpass(knk): %s\r\n", sysCfg.meshCfg.mesh_pass);
	//os_printf("\r\ngwIp(knk): %s\r\n",sysCfg.gwCfg.gateway_ip);
	os_printf("\r\ngwPort(knk): %d\r\n", sysCfg.serverCfg.server_port);

	os_printf("\r\nStaPass(knk): %s\r\n", sysCfg.sta_pwd);
	os_printf("\r\nStaSSID(knk): %s\r\n", sysCfg.sta_ssid);

}

void ICACHE_FLASH_ATTR
CFG_saveStaCfg(struct station_config sta_conf, bool save) {
	os_sprintf(sysCfg.sta_ssid, sta_conf.ssid);
	os_sprintf(sysCfg.sta_pwd, sta_conf.password);
	if (save) {
		CFG_Save();
	}
}

void ICACHE_FLASH_ATTR
CFG_saveSoftApPass(uint8_t *pass, bool save) {
	os_sprintf(sysCfg.softAp_pass, pass);
	if (save) {
		CFG_Save();
	}
}

void ICACHE_FLASH_ATTR
CFG_saveMeshConfig(MeshConfig meshCfg, bool save) {
	os_memcpy(sysCfg.meshCfg.mesh_group_id, meshCfg.mesh_group_id,
			MESH_GROUP_ID_LEN);
	os_sprintf(sysCfg.meshCfg.mesh_ssid_prefix, "%s", meshCfg.mesh_ssid_prefix);
	os_sprintf(sysCfg.meshCfg.mesh_pass, "%s", meshCfg.mesh_pass);
	sysCfg.meshCfg.isMeshEnable = meshCfg.isMeshEnable;
	if (save) {
		CFG_Save();
	}
}

void ICACHE_FLASH_ATTR
CFG_saveServerConfig(ServerConfig serverCfg , bool save) {
	os_bzero(serverCfg.server_host, sizeof(serverCfg.server_host));
	os_sprintf(sysCfg.serverCfg.server_host, "%s", serverCfg.server_host);
	sysCfg.serverCfg.server_port = serverCfg.server_port;
	if (save) {
		CFG_Save();
	}
}
void ICACHE_FLASH_ATTR
CFG_saveDeviceData(DeviceData deviceData, bool save) {

	if (save) {
		CFG_Save();
	}
}

void ICACHE_FLASH_ATTR
CFG_saveDeviceConfig(DeviceConfig deviceCfg, bool save) {
	sysCfg.deviceCfg.comMode = deviceCfg.comMode;
	os_bzero(sysCfg.deviceCfg.deviceType,sizeof(sysCfg.deviceCfg.deviceName));
	os_sprintf(sysCfg.deviceCfg.deviceType, deviceCfg.deviceType);//
	os_bzero(sysCfg.deviceCfg.deviceName,sizeof(sysCfg.deviceCfg.deviceName));
	os_sprintf(sysCfg.deviceCfg.deviceName, deviceCfg.deviceName);
	if (save) {
		CFG_Save();
	}
}


LOCAL uint8_t ICACHE_FLASH_ATTR
hex2int(uint8_t *hex, int from, int len) {
    uint32_t val = 0;
    int i = 0;
    for (i = from; i<from+len; i++) {
        // get current character then increment
        uint8_t byte = hex[i];
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

bool ICACHE_FLASH_ATTR
str_mac_to_byte_mac(uint8_t *str_mac, uint8_t *byte_mac){
	if(os_strlen(str_mac)==17){
		uint8_t i = 0;
		//os_printf("String Mac: %s\r\nByte Mac: ",str_mac);
		for(i=0;i<6;i++){
			byte_mac[i] = hex2int(str_mac, i*3, 2);//5c:cf:7f:1b:3f:e2//0:3:6:9:12:15
			//os_printf("%02x:", byte_mac[i]);
		}
		//os_printf("\r\n");
		return true;
	}
	else{
		return false;
	}
}


// A utility function to check whether x is numeric
LOCAL bool ICACHE_FLASH_ATTR
isNumericChar(uint8_t x)
{
    return (x >= '0' && x <= '9')? true: false;
}


bool ICACHE_FLASH_ATTR
str_ip_to_byte_ip(uint8_t *str_ip, uint8_t *byte_ip){
	uint8_t len = os_strlen(str_ip);
	if(len<=15){
		uint8_t str_indx = 0;
		uint8_t byte_indx = 0;
		os_bzero(byte_ip,4);
		for(str_indx=0;str_indx<len;str_indx++){
			if(isNumericChar(str_ip[str_indx])){
				byte_ip[byte_indx] *=10;
				byte_ip[byte_indx] += str_ip[str_indx] - '0';
			} else {
				byte_indx++;
			}
		}
		return true;
	} else{
		return false;
	}

}




// A simple atoi() function. If the given string contains
// any invalid character, then this function returns 0
int ICACHE_FLASH_ATTR
Atoi(uint8_t *str)
{
    if (os_strlen(str) == 0)
       return -1;

    int res = 0;  // Initialize result
    uint8_t i = 0;  // Initialize index of first digit

    // Iterate through all digits of input string and update result
    for (; str[i] != '\0'; ++i)
    {
        if (isNumericChar(str[i]) == false)
            return -1; // You may add some lines to write error message
                      // to error stream
        res = res*10 + str[i] - '0';
    }

    // Return result with sign
    return res;
}
