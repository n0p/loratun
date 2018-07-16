#include "schc.h"
#include "context.h"

/*
struct field_values sample_packet={
	0x60, 0x0A, 0x44, 0xD6, 0x00, 0x0E, 0x11, 0x40,
	0xFD, 0x73, 0xAB, 0x44, 0x93, 0xDD, 0x6B, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFD, 0x73, 0xAB, 0x44, 0x93, 0xDD, 0x6B, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0xB7, 0xB8, 0xAA, 0xAA, 0x00, 0x0E, 0x98, 0x81,
	0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00
};
*/

struct field_values sample_packet = {
	6,
	0,
	0,
	SIZE_UDP+4,
	17,
	255,
	{0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x00, 0x27, 0xff, 0xfe, 0x00, 0x00, 0x00},
	{0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x1b, 0x8f, 0xf9, 0xa3, 0x84, 0x9b, 0xb3, 0x17},
	10001,
	10001,
	SIZE_UDP+4,
	0x1234,
	{0x61, 0x62, 0x63, 0x00} // abc\0
};

/*
		// Field;               FL; FP;DI; TV;                 MO;     CA;
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "1b8ff9a3849bb317", EQUALS, NOT_SENT         },
		{ UDP_DEVPORT,         16, 1, BI, "10001",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "10001",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},
*/
int main() {

	//sample_packet.udp_dev_port = 6666;
	//sample_packet.udp_app_port = 4567;

	int ret=schc_compress(&sample_packet);
	printf("*******ret=%d\n", ret);
	return 0;
}