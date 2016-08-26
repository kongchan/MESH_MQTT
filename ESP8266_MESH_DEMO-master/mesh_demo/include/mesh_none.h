#ifndef __MESH_NONE_H__
#define __MESH_NONE_H__

#include "c_types.h"
#include "mesh.h"
/*
 * this function is used to parse mesh (topology)
 */
void mesh_none_proto_parser(uint8_t *mesh_header, uint8_t *pdata, uint16_t len);
void ICACHE_FLASH_ATTR udp_mesh_M_PROTO_NONE(const uint8_t *pdata);

#endif
