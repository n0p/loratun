#ifndef __SCHC_H
#define __SCHC_H

/**
 * \file
 * 
 * \brief SCHC C/D related functions and struct definition.
 *
 * All the definitions in this file are directly from the
 * draft-ietf-lpwan-ipv6-static-context-hc-10.
 */

/**********************************************************************/
/***        Include files                                           ***/
/**********************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/**********************************************************************/
/***        Local Include files                                     ***/
/**********************************************************************/

/**********************************************************************/
/***        Macro Definitions                                       ***/
/**********************************************************************/

#define SIZE_ETHERNET 14
#define SIZE_IPV6 40
#define SIZE_UDP 8
#define SIZE_MTU_IPV6 1280
/**
 * This is taken from the LoRaWAN Specification v1.0 Table 17.
 *
 * Not really a generic solution, since every LPWAN has a different PTU,
 * but for now it servers as an ad-hoc solution for testings.
 */
#define MAX_SCHC_PKT_LEN 242

// Fragmentation/Reassembly {

/**
 * This is the maximum size of the payload of each SCHC Fragment.
 */
#define SCHC_FRAG_PAY_LEN 10 /* TODO this value is set for testing, change to something else */

// } Fragmentation/Reassembly

/**********************************************************************/
/***        Types Definitions                                       ***/
/**********************************************************************/

// SCHC draft 10, section 6.4
enum MO {
	EQUALS,
	IGNORE,
	MATCH_MAPPING,
	MSB
};

// SCHC draft 10, section 9
enum fieldid {
	IPV6_VERSION,
	IPV6_TRAFFIC_CLASS,
	IPV6_FLOW_LABEL,
	IPV6_PAYLOAD_LENGTH,
	IPV6_NEXT_HEADER,
	IPV6_HOP_LIMIT,
	IPV6_DEV_PREFIX,
	IPV6_DEVIID,
	IPV6_APP_PREFIX,
	IPV6_APPIID,
	UDP_DEVPORT,
	UDP_APPPORT,
	UDP_LENGTH,
	UDP_CHECKSUM
};

// SCHC draft 10, section 6.1
enum direction {
	UPLINK,
	DOWNLINK,
	BI
};

// SCHC draft 10, section 6.5
enum CDA {
	NOT_SENT,
	VALUE_SENT,
	MAPPING_SENT,
	LSB,
	COMPUTE_LENGTH,
	COMPUTE_CHECKSUM,
	DEVIID,
	APPIID
};


struct field_description {
	enum fieldid fieldid;
	size_t field_length; /** Length in bits */
	int field_position;
	enum direction direction;
	char *tv;
	enum MO MO;
	enum CDA CDA;
};

struct field_values {
	uint8_t ipv6_version;
	uint8_t ipv6_traffic_class;
	uint32_t ipv6_flow_label;
	size_t ipv6_payload_length;
	uint8_t ipv6_next_header;
	uint8_t ipv6_hop_limit;
	uint8_t ipv6_dev_prefix[8];
	uint8_t ipv6_dev_iid[8];
	uint8_t ipv6_app_prefix[8];
	uint8_t ipv6_app_iid[8];

	uint16_t udp_dev_port;
	uint16_t udp_app_port;
	size_t udp_length;
	uint16_t udp_checksum;
	
	uint8_t payload[SIZE_MTU_IPV6]; 
};

/**********************************************************************/
/***        Forward Declarations                                    ***/
/**********************************************************************/

/**
 * \brief Applies the SCHC compression procedure as detailed in
 * draft-ietf-lpwan-ipv6-static-context-hc-10 and, in case of success,
 * sends the SCHC Packet or derived SCHC Fragments though the downlink.
 *
 * \note In case of failure or not matching any SCHC Rule, the packet is
 * silently discarded and is not sent through the downlink.
 *
 * @param [in] ipv6_pcaket The original IPv6 packet received from
 * the ipv6 interface. It will be used to apply a SCHC rule and generate
 * a compressed IPv6 Packet.
 *
 * @return 0 if successfull, non-zero if there was an error.
 */
int schc_compress(const struct field_values *ipv6_packet);

/*
 * Extract in fv the data of the compressed packet used the SCHC decompression
 * procedure as detailed in draft-ietf-lpwan-ipv6-static-context-hc-10.
 * @return 0 if successfull, non-zero if there was an error.
 */
int schc_decompress(uint8_t *schc_packet, uint8_t schc_packet_len, struct field_values *fv);

/**********************************************************************/
/***        Constants                                               ***/
/**********************************************************************/

/**********************************************************************/
/***        Global Variables                                        ***/
/**********************************************************************/

/**********************************************************************/
/***        AUX Functions                                           ***/
/**********************************************************************/

/**********************************************************************/
/***        MAIN routines                                           ***/
/**********************************************************************/

/**********************************************************************/
/***        END OF FILE                                             ***/
/**********************************************************************/

#endif /* SCHC_H */
