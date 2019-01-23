/*
 * sys_utilities.c
 *
 *  Created on: Oct 27, 2016
 *  Author: Istiaq Mahbub
 */
#include "sys_utilities.h"
#include <time.h>

/*void ICACHE_FLASH_ATTR
get_system_rtc_time(struct time_stamp_t *now){

	uint32_t epoch_time_offset = 1477544088;//seconds since Jan 01 1970. (UTC)
	uint32_t epoch_time_now = ((system_get_rtc_time()*system_rtc_clock_cali_proc())/1000000) + epoch_time_offset;
	now->YYYY = epoch_time_now/31536000;
}*/

