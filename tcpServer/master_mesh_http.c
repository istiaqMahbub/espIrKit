/******************************************************************************
 * Copyright 2015-2016 Espressif Systems
 *
 * FileName: mesh_http.c
 *
 * Description: mesh demo for HTTP protocol parser
 *
 * Modification history:
 *     2016/03/15, v1.1 create this file.
*******************************************************************************/
#include "master_mesh_parser.h"

void ICACHE_FLASH_ATTR
mesh_http_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len)
{
    uint16_t i = 0;
    uint16_t url_count = 0;
    const char *url_req = NULL;

    MESH_PARSER_PRINT("%s, len:%u, data:%s\n", __func__, len, pdata);
}
