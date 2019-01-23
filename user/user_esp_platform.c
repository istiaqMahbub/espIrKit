/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_esp_platform.h"




#define ESP_DEBUG

#ifdef ESP_DEBUG
#define ESP_DBG os_printf
#else
#define ESP_DBG
#endif

#define ACTIVE_FRAME    "{\"nonce\": %d,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

struct esp_platform_saved_param esp_param;
LOCAL uint8 device_status;

void ICACHE_FLASH_ATTR
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_connect_status
 * Description  : get each connection step's status
 * Parameters   : none
 * Returns      : status
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    ESP_DBG("status %d\n", status);
    return status;
}
