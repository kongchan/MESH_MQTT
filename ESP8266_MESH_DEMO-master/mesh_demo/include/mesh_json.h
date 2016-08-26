#ifndef __MESH_JSON_H__
#define __MESH_JSON_H__

#include "c_types.h"
#include "mesh.h"
/*
 * this function is used to parse packet formatted with JSON
 * you can merge cJSON into the function.
 */
void mesh_json_proto_parser(uint8_t *mesh_header, uint8_t *pdata, uint16_t len);

#endif
