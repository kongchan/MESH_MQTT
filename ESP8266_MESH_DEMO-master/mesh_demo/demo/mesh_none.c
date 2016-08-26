/******************************************************************************
 * Copyright 2015-2016 Espressif Systems
 *
 * FileName: mesh_none.c
 *
 * Description: mesh demo for mesh topology parser
 *
 * Modification history:
 *     2016/03/15, v1.1 create this file.
*******************************************************************************/
#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "mesh_parser.h"

#include "mesh.h"
#include "mqtt.h"
os_timer_t mqtt_subscribe_timer;
extern MQTT_Client *mqttClient;
extern struct espconn g_ser_conn;
extern u8 mqtt_id[11];


enum None_MyEnum
{
	Subscribe = 1,
	mqtt_topic_send = 2,
	test = 3,
	data = 4,

};
extern struct espconn g_ser_conn;

int ICACHE_FLASH_ATTR
none_mode_parse(uint8_t *pdata)
{
	char *mode = pdata;

	if (os_strstr(mode, "Subscribe"))
		return 1;
	if (os_strstr(mode, "mqtt_topic_send"))
		return 2;
	if (os_strstr(mode, "/meter/test"))
		return 3;
	if (os_strstr(mode, "/meter/data"))
		return 4;
	
	return 0;
}

void ICACHE_FLASH_ATTR
mesh_none_proto_parser(uint8_t *mesh_header, uint8_t *pdata, uint16_t len)
{
	MESH_PARSER_PRINT("len:%u, data:%s\n", len, pdata);
	if (!espconn_mesh_is_root())
	{
		MESH_PARSER_PRINT("****** non root do not none parse****\n");
		return;
	}
	/*
	解释pdata的形式，而进行对应的读取数据或者之类。再组装成mesh包发回给去。
	*/
	MESH_PARSER_PRINT("************ get none and parse *********\n");
	enum None_MyEnum mode = 0;
	mode = none_mode_parse(pdata);
	char *Subscribe_mqtt_id = pdata;
	MESH_PARSER_PRINT("********* Json_MyEnum mode = %d **********\n", &mode);
	switch (mode)
	{
	case Subscribe:
		MESH_PARSER_PRINT("**************into Subscribe parse*************\n");
		u8 temp[256];
		Subscribe_mqtt_id = Subscribe_mqtt_id + 16;
		MQTT_Subscribe(mqttClient, Subscribe_mqtt_id, 0);
		os_sprintf(temp, "%s connected", Subscribe_mqtt_id);
		MQTT_Publish(mqttClient, "/meter/info", temp, strlen(temp), 0, 0);
		struct mesh_header_format *header = (struct mesh_header_format *)mesh_header;
		p2p_mesh_json(header, "Subscribed", len);
		MESH_DEMO_FREE(header);
		break;

	case mqtt_topic_send:
		MESH_PARSER_PRINT("**************into mqtt_topic_send parse*************\n");

		Subscribe_mqtt_id = Subscribe_mqtt_id + 22;
		*(Subscribe_mqtt_id + 11) = '\0';
		os_printf("************ Subscribe_mqtt_id: %s **********\n", Subscribe_mqtt_id);
		pdata = pdata + 34;
		os_printf("************ pdata: %s **********\n", pdata);

		MQTT_Publish(mqttClient, Subscribe_mqtt_id, pdata, strlen(pdata), 0, 0);
		break;

	case test:
		MESH_PARSER_PRINT("**************into test parse*************\n");
		pdata = pdata + 18;
		MESH_PARSER_PRINT("********** v9881_data->jsonString: %s **********\n", pdata);
		MQTT_Publish(mqttClient, "/meter/test", pdata, strlen(pdata), 0, 0);
		break;

	case data:
		MESH_PARSER_PRINT("**************into data parse*************\n");
		pdata = pdata + 18;
		MESH_PARSER_PRINT("********** v9881_data->jsonString: %s **********\n", pdata);
		MQTT_Publish(mqttClient, "/meter/data", pdata, strlen(pdata), 0, 0);
		break;

	default:
		MESH_PARSER_PRINT("******** default ********\n");
		break;
	}
}

void ICACHE_FLASH_ATTR
mesh_disp_sub_dev_mac(uint8_t *sub_mac, uint16_t sub_count)
{
	uint16_t i = 0;
	const uint8_t mac_len = 6;

	if (!sub_mac || sub_count == 0) {
		MESH_PARSER_PRINT("sub_mac:%p, sub_count:%u\n", sub_mac, sub_count);
		return;
	}

	for (i = 0; i < sub_count; i++) {
		MESH_PARSER_PRINT("idx: %u, mac:" MACSTR "\n", i, MAC2STR(sub_mac));
		sub_mac += mac_len;
	}
}

void ICACHE_FLASH_ATTR mesh_topo_test()
{
    uint8_t src[6];
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;
    struct mesh_header_option_format *option = NULL;
    uint8_t ot_len = sizeof(struct mesh_header_option_header_type) + sizeof(*option) + sizeof(dst); 

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PARSER_PRINT("get sta mac fail\n");
        return;
    }

    /*
     * root device uses espconn_mesh_get_node_info to get mac address of all devices      
     */
    if (espconn_mesh_is_root()) {
        uint8_t *sub_dev_mac = NULL;
        uint16_t sub_dev_count = 0;
        if (!espconn_mesh_get_node_info(MESH_NODE_ALL, &sub_dev_mac, &sub_dev_count))
            return;
        // the first one is mac address of router
        mesh_disp_sub_dev_mac(sub_dev_mac, sub_dev_count);
        mesh_device_set_root((struct mesh_device_mac_type *)src);
        if (sub_dev_count > 1) {
            struct mesh_device_mac_type *list = (struct mesh_device_mac_type *)sub_dev_mac;
            mesh_device_add(list + 1, sub_dev_count - 1);
        }

        // release memory occupied by mac address.
        espconn_mesh_get_node_info(MESH_NODE_ALL, NULL, NULL); 
        return;
    }

    /*
     * non-root device uses topology request with bcast to get mac address of all devices
     */
//    os_memset(dst, 0, sizeof(dst));  // use bcast to get all the devices working in mesh from root.
//    header = (struct mesh_header_format *)espconn_mesh_create_packet(
//                            dst,     // destiny address (bcast)
//                            src,     // source address
//                            false,   // not p2p packet
//                            true,    // piggyback congest request
//                            M_PROTO_NONE,  // packe with JSON format
//                            0,       // data length
//                            true,    // no option
//                            ot_len,  // option total len
//                            false,   // no frag
//                            0,       // frag type, this packet doesn't use frag
//                            false,   // more frag
//                            0,       // frag index
//                            0);      // frag length
//    if (!header) {
//        MESH_PARSER_PRINT("create packet fail\n");
//        return;
//    }
//
//    option = (struct mesh_header_option_format *)espconn_mesh_create_option(
//            M_O_TOPO_REQ, dst, sizeof(dst));
//    if (!option) {
//        MESH_PARSER_PRINT("create option fail\n");
//        goto TOPO_FAIL;
//    }
//
//    if (!espconn_mesh_add_option(header, option)) {
//        MESH_PARSER_PRINT("set option fail\n");
//        goto TOPO_FAIL;
//    }
//
//    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
//        MESH_PARSER_PRINT("mesh is busy\n");
//        espconn_mesh_connect(&g_ser_conn);
//    }
//TOPO_FAIL:
//    option ? MESH_DEMO_FREE(option) : 0;
//    header ? MESH_DEMO_FREE(header) : 0;
}

void ICACHE_FLASH_ATTR mesh_topo_test_init()
{
    static os_timer_t topo_timer;
    os_timer_disarm(&topo_timer);
    os_timer_setfn(&topo_timer, (os_timer_func_t *)mesh_topo_test, NULL);
    os_timer_arm(&topo_timer, 14000, true);
}

void ICACHE_FLASH_ATTR mqtt_subscribe_udp()
{
	char *tst_data = (char *)os_malloc(256 * sizeof(char));
	os_sprintf(tst_data, "Subscribe:");
	os_sprintf(tst_data + os_strlen(tst_data), "%s", mqtt_id);
	MESH_PARSER_PRINT("****** into the mqtt_subscribe_udp ****\n");
	udp_mesh_M_PROTO_NONE(tst_data);
	MESH_DEMO_FREE(tst_data);
}

void ICACHE_FLASH_ATTR mqtt_subscribe_init()
{

	os_timer_disarm(&mqtt_subscribe_timer);
	os_timer_setfn(&mqtt_subscribe_timer, (os_timer_func_t *)mqtt_subscribe_udp, NULL);
	os_timer_arm(&mqtt_subscribe_timer, 5000, true);
}



void ICACHE_FLASH_ATTR udp_mesh_M_PROTO_NONE(const uint8_t *pdata)
{
	char buf[32];
	uint8_t src[6];
	uint8_t dst[6];
	struct mesh_header_format *header = NULL;

	if (!wifi_get_macaddr(STATION_IF, src)) {
		MESH_PARSER_PRINT("bcast get sta mac fail\n");
		return;
	}

	os_memset(buf, 0, sizeof(buf));
	//os_sprintf(buf, "%s", "{\"bcast\":\"");
	//os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
	//os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");
	os_sprintf(buf, "%s", "bcast:");
	os_sprintf(buf + os_strlen(buf), "%s", pdata);

	os_memset(dst, 0, sizeof(dst));  // use bcast to get all the devices working in mesh from root.
	header = (struct mesh_header_format *)espconn_mesh_create_packet(
		dst,     // destiny address (bcast)
		src,     // source address
		false,   // not p2p packet
		true,    // piggyback congest request
		M_PROTO_NONE,   // packe with JSON format
		os_strlen(buf), // data length
		false,   // no option
		0,       // option total len
		false,   // no frag
		0,       // frag type, this packet doesn't use frag
		false,   // more frag
		0,       // frag index
		0);      // frag length
	if (!header) {
		MESH_PARSER_PRINT("bcast create packet fail\n");
		return;
	}

	if (!espconn_mesh_set_usr_data(header, buf, os_strlen(buf))) {
		MESH_DEMO_PRINT("bcast set user data fail\n");
		MESH_DEMO_FREE(header);
		return;
	}

	if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
		MESH_DEMO_PRINT("bcast mesh is busy\n");
		espconn_mesh_connect(&g_ser_conn);
		MESH_DEMO_FREE(header);
		return;
	}

	MESH_DEMO_FREE(header);
}
