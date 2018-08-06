/**
 * \file
 * \brief Implementation of the schc.h functions.
 *
 * This file implements the SCHC compression/decompression (SCHC C/D)
 * actions as defined in draft-ietf-lpwan-ipv6-static-context-hc-10. The
 * most important functions in this file are schc_compress() and
 * schc_decompress().
 *
 * \note Only a subset of operations of the SCHC C/D are implemented,
 * not all of them.
 *
 * \note The SCHC Fragmentation/Reassembly (SCHC F/R) is not
 * implemented.
 *
 * \note The rule Field Length is not used at all in this
 * implementation, we hardcoded everything in the struct
 * field_description. Solving this requires thinking.
 *
 * To understand better the structure of a schc_packet:
 *
 * \verbatim
 * +--- ... --+------- ... -------+------------------+~~~~~~~
 * |  Rule ID |Compression Residue| packet payload   | padding
 * +--- ... --+------- ... -------+------------------+~~~~~~~
 *                                                    (optional)
 * <----- compressed header ------>
 *
 * Figure 6: from the draft-ietf-lpwan-ipv6-static-context-hc-10
 *
 * \endverbatim
 *
 * \related schc_compress
 */

/**********************************************************************/
/***        Include files                                           ***/
/**********************************************************************/

#include <stdio.h>
#include <string.h> /* To use memcmp() */
#include <arpa/inet.h> /* inet_pton(), htonl(), htons() */
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <bsd/string.h> /* strlcpy */

/**********************************************************************/
/***        Local Include files                                     ***/
/**********************************************************************/

#include "schc.h"
#include "context.h" /* To use rules[][14] */
//#include "ipv6sniffer.h" /* To use rules[][14] */

/*
 * Once we receive a IPv6 packet, we apply compression and send it to the
 * LoRaWAN server downlink queue of the device.
 */
//#include "lorawanmqtt.h" 

/**********************************************************************/
/***        Macro Definitions                                       ***/
/**********************************************************************/
//#define DEBUG // Activate this macro to print debug messages.

#ifdef DEBUG

	#define PRINTF(...) printf(__VA_ARGS__)
	#define PRINT_ARRAY(add, len)                             \
	do {                                                      \
		int i;                                                 \
		for (i = 0 ; i < (len) ; i++) {                        \
			printf("%02X ", (unsigned int)((uint8_t*)(add))[i]); \
		}                                                      \
		printf("\n");                                          \
	} while(0)

#else /* DEBUG */

	#define PRINTF(...)
	#define PRINT_ARRAY(add, len)

#endif /* DEBUG */

/**********************************************************************/
/***        Types Definitions                                       ***/
/**********************************************************************/

/**********************************************************************/
/***        Forward Declarations                                    ***/
/**********************************************************************/

/**********************************************************************/
/***        Constants                                               ***/
/**********************************************************************/

/**********************************************************************/
/***        Global Variables                                        ***/
/**********************************************************************/

uint16_t compute_checksum = 0;
uint16_t compute_length = 0;

/**********************************************************************/
/***        Static Functions                                        ***/
/**********************************************************************/

/*
static int schc_fragmentate(const uint8_t *schc_packet, size_t schc_packet_len)
{

	/x*
	 * If the packet len is equal or less than the max size of a
	 * L2 packet, we send the packet as is, without fragmentation
	 *x/

	if (schc_packet_len <= MAX_SCHC_PKT_LEN) {

		lorawanmqtt_msg m = {

			/x* TODO Application hardcoded, está mal. *x/
			.applicationID = LORAWAN_APPLICATION_ID,
			.fport = 1,
			.reference = "condor",
			.confirmed = 1
		};

		/x* TODO LORAWAN_DEVEUI hardcoded, está mal. *x/
		strlcpy(m.deveui, LORAWAN_DEVEUI, sizeof(m.deveui));


		memcpy(m.payload, schc_packet, schc_packet_len);
		m.payload_len = schc_packet_len;

		if (lorawanmqtt_send(&m) != 0)
			return -1;

		return 0;

	}

	return -1;


}
*/




uint16_t checksum (uint16_t *addr, int len) {

	PRINTF("checksum -> calculating checksum\n");

	int count = len;
	register uint32_t sum = 0;
	uint16_t answer = 0;
	uint8_t first,second;

	// Sum up 2-byte values until none or only one byte left.
	while (count > 1) {
		sum += *(addr++);
		count -= 2;
	}

	// Add left-over byte, if any.
	if (count > 0) {
		sum += *(uint8_t *) addr;
	}

	// Fold 32-bit sum into 16 bits; we lose information by doing this,
	// increasing the chances of a collision.
	// sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	first = (sum >> 8) & 255;
	second = (sum ) & 255;

	sum = 0;

	sum |= (second << 8);
	sum |= first << 0; 
	// Checksum is one's compliment of sum.
	answer = ~sum;

	PRINTF("checksum -> checksum calculated: 0x%x\n", answer);

	return (answer);
}


uint16_t checksum_udp_ipv6(char *buffer, int length_buffer) {

	PRINTF("checksum_udp_ipv6 -> preparing pseudo-header\n");

	uint8_t aux[255] = {0};
	uint8_t aux_offset = 0;
	int i = 0;

    /*
     * Pseudo Header.
     */
    {

		 /* IPv6 Addresses */
		 for (i = 0 ; i < 32 ; i++ ) {
			aux[aux_offset] = buffer[8+i];
			aux_offset += 1;

		 }

        /* protocol (UDP 0x11) */
		aux[aux_offset] = 0x0;
		aux[aux_offset+1] = 0x11;
		aux_offset += 2;

        /* payload length (UDP Header + Payload ) */
		aux[aux_offset] = buffer[4];
		aux[aux_offset+1] = buffer[5];
		aux_offset += 2;

    } /* Pseudo header end */

	uint16_t udp_payload_length = length_buffer - 40;
	for (i = 0; i < udp_payload_length ; i++) {
		aux[aux_offset] = buffer[40+i];
		aux_offset += 1;
	}

	PRINTF("checksum_udp_ipv6 -> pseudo-header ready\n");

	return checksum((uint16_t*)aux, aux_offset);

}








/**
 * \brief Appends the compression residue to the SCHC packet, using the
 * CA action defined in rule_row.
 *
 * \note That this function might not append any bytes to the
 * compression residue. Such thing is possible depending on the rule.
 *
 * @param [in] rule_row The rule row to check the Compression Action
 * (CA) to do to the ipv6_packet target value. Must not be NULL.
 *
 * @param [in] ipv6_packet The original ipv6_packet from wihch we
 * extract the information in case we need to copy some information to
 * the compression residue.
 *
 * @param [in,out] schc_packet The target SCHC compressed packet result of
 * aplying the CA. The function appends as many bytes as it needs
 * depending on the rule_row->CDA. Must be not-null. If the function
 * returns error, the contents of the schc_packet are undefined.
 *
 * @param [in,out] offset The position in schc_packet where the
 * function must append bytes to the Compression Residue. Once the
 * function returns, it will point to the next available byte in the
 * schc_packet. The idea is to call this function many times in
 * sequence, every call will append new bytes to the schc_packet, and
 * after the last call, it points where the application payload should
 * be.
 *
 * @return Zero if success, non-zero if error.
 *
 * \note Only implements the following CA:
 * - COMPUTE_LENGTH
 * - COMPUTE_CHECKSUM
 * - NOT_SENT
 *   TODO implement the rest.
 *
 * We just need a quick implementation to start testing with a real
 * scenario, so we don't bother with all the complex compression
 * actions, like the MSB or Map Matching. We need an ad-hoc solution for
 * our tests.
 *
 */
static int do_compression_action(
	const struct field_description *rule_row,
	const struct field_values *ipv6_packet,
	uint8_t schc_packet[SIZE_MTU_IPV6], size_t *offset
) {

	if (rule_row == NULL || ipv6_packet == NULL) {
		return -1;
	}

	if (rule_row->CDA == COMPUTE_LENGTH || rule_row->CDA == NOT_SENT ||
	    rule_row->CDA == COMPUTE_CHECKSUM) {
		return 0;
	}

	size_t n = 0;

	if (rule_row->CDA == VALUE_SENT) {

		switch (rule_row->fieldid) {
			case IPV6_VERSION:
				n = 1;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_version, n);
				*offset += n;
				return 0;
			case IPV6_TRAFFIC_CLASS:
				n = 1;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_traffic_class, n);
				*offset += n;
				return 0;
			case IPV6_FLOW_LABEL:
				n = 3;
				uint32_t udp_flow_label = htonl(ipv6_packet->ipv6_flow_label);
				udp_flow_label = udp_flow_label << 8;
				memcpy(&schc_packet[*offset], &udp_flow_label, n);
				*offset += n;
				return 0;
			case IPV6_PAYLOAD_LENGTH:
				n = 2;
				uint16_t ipv6_payload_length = htons(ipv6_packet->ipv6_payload_length);
				memcpy(&schc_packet[*offset], &ipv6_payload_length, n);
				*offset += n;
				return 0;
			case IPV6_NEXT_HEADER:
				n = 1;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_next_header, n);
				*offset += n;
				return 0;
			case IPV6_HOP_LIMIT:
				n = 1;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_hop_limit, n);
				*offset += n;
				return 0;
			case IPV6_DEV_PREFIX:
				n = 8;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_dev_prefix, n);
				*offset += n;
				return 0;
			case IPV6_DEVIID:
				n = 8;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_dev_iid, n);
				*offset += n;
				return 0;
			case IPV6_APP_PREFIX:
				n = 8;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_app_prefix, n);
				*offset += n;
				return 0;
			case IPV6_APPIID:
				n = 8;
				memcpy(&schc_packet[*offset], &ipv6_packet->ipv6_app_iid, n);
				*offset += n;
				return 0;
			case UDP_DEVPORT:
				n = 2;
				uint16_t udp_dev_port = htons(ipv6_packet->udp_dev_port);
				memcpy(&schc_packet[*offset], &udp_dev_port, n);
				*offset += n;
				return 0;
			case UDP_APPPORT:
				n = 2;
				uint16_t udp_app_port = htons(ipv6_packet->udp_app_port);
				memcpy(&schc_packet[*offset], &udp_app_port, n);
				*offset += n;
				return 0;
			case UDP_LENGTH:
				n = 2;
				uint16_t udp_length = htons(ipv6_packet->udp_length);
				memcpy(&schc_packet[*offset], &udp_length, n);
				*offset += n;
				return 0;
			case UDP_CHECKSUM:
				n = 2;
				uint16_t udp_checksum = htons(ipv6_packet->udp_checksum);
				memcpy(&schc_packet[*offset], &udp_checksum, n);
				*offset += n;
				return 0;
			default:
				break;
		}
	}

	return -1;
}

/*
 * TODO check argument sanity
 * TODO documentar
 */
static int string_to_bin(uint8_t dst[8], const char *src)
{

	/* WARNING: no sanitization or error-checking whatsoever */
	for (size_t count = 0; count < 8; count++) {
		sscanf(src, "%2hhx", &dst[count]);
		src += 2;
	}

	return 0;
}

/**
 * \brief Checks the corresponding fv value on the fd->fieldid, using
 * the fd->MO against fd->tv.
 *
 * @param [in] rule_row The rule row in the context to check the MO and
 * the TV. Must be non-zero, or returns 0.
 *
 * @param [in] ipv6_packet The received IPv6 packet to check for matching. Must
 * be non-zero, or returns 0.
 *
 * @return 1 if the field matches with the MO. 0 if the field does not
 * match.
 *
 * The idea of this function is to be called in a loop from
 * schc_compress() for any rows in the SCHC rule and check individually
 * if it's a match or not.
 *
 * I don't really like the way it works, the switch (fd->fieldid) is a
 * little dirty, but right now it does the job.
 *
 * \note Does not distinguish between an error an a non-match. In both
 * cases, 0 is returned.
 *
 * TODO It must distinguis between an error an a non-match.
 */
static int check_matching(const struct field_description *rule_row, const struct field_values *ipv6_packet)
{

	PRINTF("check_matching entro\n");

	if (rule_row == NULL || ipv6_packet == NULL) {
		return 0;
	}

	if (rule_row->MO == IGNORE) {
		return 1;
	}

	if (rule_row->MO == EQUALS) {
		uint8_t tv[8] = {0};

		switch (rule_row->fieldid) {

			case IPV6_VERSION:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_version);
			case IPV6_TRAFFIC_CLASS:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_traffic_class);
			case IPV6_FLOW_LABEL:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_flow_label);
			case IPV6_PAYLOAD_LENGTH:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_payload_length);
			case IPV6_NEXT_HEADER:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_next_header);
			case IPV6_HOP_LIMIT:
				return (atoi(rule_row->tv) == ipv6_packet->ipv6_hop_limit);
			case IPV6_DEV_PREFIX:

				/* TODO return value checking */
				string_to_bin(tv, rule_row->tv);

				PRINTF("IPV6_DEV_PREFIX\n");
				PRINTF("Should be: \n");
				PRINT_ARRAY(tv, 8);
				PRINTF("Actualy is: \n");
				PRINT_ARRAY(ipv6_packet->ipv6_dev_prefix, 8);

				/*
				 * If the field is not equal, we must return error.
				 */
				if (memcmp(ipv6_packet->ipv6_dev_prefix, tv, 8) != 0) {
					PRINTF("TV NOT EQUALS\n");
					return 0;
				}
				return 1;
			case IPV6_DEVIID:
				/* TODO return value checking */
				string_to_bin(tv, rule_row->tv);

				PRINTF("IPV6_DEV_IID\n");
				PRINTF("Should be: \n");
				PRINT_ARRAY(tv, 8);
				PRINTF("Actualy is: \n");
				PRINT_ARRAY(ipv6_packet->ipv6_dev_iid, 8);

				/*
				 * If the field is not equal, we must return error.
				 */
				if (memcmp(ipv6_packet->ipv6_dev_iid, tv, 8) != 0) {
					PRINTF("TV NOT EQUALS\n");
					return 0;
				}
				return 1;
			case IPV6_APP_PREFIX:
				/* TODO return value checking */
				string_to_bin(tv, rule_row->tv);

				PRINTF("IPV6_APP_PREFIX\n");
				PRINTF("Should be: \n");
				PRINT_ARRAY(tv, 8);
				PRINTF("Actualy is: \n");
				PRINT_ARRAY(ipv6_packet->ipv6_app_prefix, 8);

				/*
				 * If the field is not equal, we must return error.
				 */
				if (memcmp(ipv6_packet->ipv6_app_prefix, tv, 8) != 0) {
					PRINTF("TV NOT EQUALS\n");
					return 0;
				}
				return 1;
			case IPV6_APPIID:
				/* TODO return value checking */
				string_to_bin(tv, rule_row->tv);

				PRINTF("IPV6_APP_IID\n");
				PRINTF("Should be: \n");
				PRINT_ARRAY(tv, 8);
				PRINTF("Actualy is: \n");
				PRINT_ARRAY(ipv6_packet->ipv6_app_iid, 8);

				/*
				 * If the field is not equal, we must return error.
				 */
				if (memcmp(ipv6_packet->ipv6_app_iid, tv, 8) != 0) {
					PRINTF("TV NOT EQUALS\n");
					return 0;
				}
				return 1;
			case UDP_DEVPORT:
				return (atoi(rule_row->tv) == ipv6_packet->udp_dev_port);
			case UDP_APPPORT:
				return (atoi(rule_row->tv) == ipv6_packet->udp_app_port);
			case UDP_LENGTH:
				return (atoi(rule_row->tv) == ipv6_packet->udp_length);
			case UDP_CHECKSUM:
				return (atoi(rule_row->tv) == ipv6_packet->udp_checksum);
			default:
				break;

		}

	}

	return 0;
}

/**********************************************************************/
/***        Global Functions                                        ***/
/**********************************************************************/


/**
 * To understand what this function does it's essential that you
 * familiarize yourself with the SCHC draft, please, see the references
 * in the README.rst file. If you're too busy, it can be summarized in:
 *
 * - Both ends of the Compression/Decompression procedure share a common
 *   context. It is a set of rules that have different match operators.
 *   In this implementation, that shared context is stored in context.c.
 *
 * - With the information in the context and the original IPv6 packet
 *   information, conveniently structured in fv, you can get a
 *   compressed packet, called schc_packet. Semantically it's like this:
 *
 *   schc_packet = schc_compress(original ipv6 packet, context).
 *
 *   On the other end, decompression goes as follows:
 *
 *   original ipv6 packet = schc_decompression(schc_packet, context).
 *
 *   Note that the context doesn't change during the duration of the
 *   lifetime of the mote.
 *
 * This is one of the core functions of the project and a lot of though
 * must be put into any modification made to it. Think carefully what
 * you want to do before changing the behavior.
 *
 * This function will be called whenever a ipv6 packet comes through the
 * network interface. The resulting schc_packet is what will be sent
 * over the constrained radio link (e.g. LoRaWAN, Sigfox).
 *
 * Please, we encourage you to go and read the
 * draft-ietf-lpwan-ipv6-static-context-hc-10 before trying to do any
 * development to this function. It is really important that you
 * understand the draft and all the concepts in play.
 *
 * \note We don't make any sanity checking of the parameters, a sane
 * structured as described in context.c is assumed.
 *
 * \note If something goes wrong during the procedure, the function
 * stops as soon as it finds the error and does nothing else.
 *
 */

int schc_compress(const struct field_values *ipv6_packet, uint8_t *schc_packet2, size_t *schc_packet_len2)
{

	uint8_t schc_packet[SIZE_MTU_IPV6] = {0};
	size_t  schc_packet_len = 0;

	/*
	 * Pointer to the current location of the schc_packet.
	 */
	uint8_t offset = 0;

	PRINTF("schc_compress - we enter the function\n");

	PRINT_ARRAY(ipv6_packet, 80);

	/*
	 * We go through all the rules.
	 */

	int nrules = sizeof(rules) / sizeof(rules[0]);

	for (int i = 0 ; i < nrules ; i++ ) {

		struct field_description current_rule[14] = {0};
		memcpy(&current_rule, rules[i], sizeof(current_rule));

		/*
		 * number of field_descriptors in the rule.
		 */
		int rule_rows = sizeof(current_rule) / sizeof(current_rule[0]);

		//   /* Field;               FL; FP;DI; TV;                 MO;     CA; */
		// 	{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		// 	{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		// 	{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                EQUALS, NOT_SENT         },
		// 	{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		// 	{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		// 	{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		// 	{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		// 	{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		// 	{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		// 	{ IPV6_APPIID,         64, 1, BI, "b1b2377fef871058", EQUALS, NOT_SENT         },
		// 
		// 	{ UDP_DEVPORT,         16, 1, BI, "8720",             EQUALS, NOT_SENT         },
		// 	{ UDP_APPPORT,         16, 1, BI, "8720",             EQUALS, NOT_SENT         },
		// 	{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		// 	{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }

		int rule_matches = 1; /* Guard Condition for the next loop */

		for (int j = 0 ; j < rule_rows ; j++) {

			/*
			 * We check all fields to see if they match until. If some condition is not
			 * met, we stop and return an error.
			 */
			if (check_matching(&current_rule[j], ipv6_packet) == 0) {
				rule_matches = 0;
				break;
			}

		}

		if (rule_matches == 0) {
			schc_packet_len = 0;
			continue;
		}


		/*
		 * At this point, all the MO of the rule returned success, we can
		 * start writing the schc_packet.
		 *
		 * First, we append the Rule ID to the schc_packet
		 */
		schc_packet[schc_packet_len] = i;
		schc_packet_len++;

		/*
		 * We do the compression actions.
		 */

		for (int j = 0 ; j < rule_rows ; j++) {

			/*
			 * We apply the compression action 
			 */
			if (do_compression_action(&current_rule[j], ipv6_packet, schc_packet, &schc_packet_len) != 0) {
				return -1;
			}

		}

		/*
		 * We do the compression actions.
		 */

		/*
		 * At this point, Compression Ressidue is already in the packet.
		 * We concatenate the app_payload to the packet.
		 */

		size_t app_payload_len = ipv6_packet->udp_length - SIZE_UDP;
		if (app_payload_len<0) {
			PRINTF("PAYLOAD_LEN < 0\n");
			return -1;
		}

		uint8_t *p = schc_packet + schc_packet_len;

		memcpy(p, ipv6_packet->payload, app_payload_len);

		schc_packet_len += app_payload_len;

		PRINTF("schc_compression() result: \n");
		PRINT_ARRAY(schc_packet, schc_packet_len);

		/*
		 * We have created the SCHC packet, we now go to the Fragmentation
		 * layer and the packet will be sent to the downlink LPWAN tech by
		 * the schc_fragmentate() function as a whole SCHC packet, or as a 
		 * series of fragments.
		 *
		 * If schc_fragmentate succeeds, we return succeed. If it fails,
		 * we return fail.
		 */
		//return schc_fragmentate(schc_packet, schc_packet_len);

		// copy
		memcpy(schc_packet2, (uint8_t *)&schc_packet, schc_packet_len);
		*schc_packet_len2=schc_packet_len;

		return 0;

	}

	/*
	 * No Rule in the context matched the ipv6_packet.
	 */
	return -1;

}


uint16_t compute_udp_checksum (struct field_values *fv) {

	struct ip6_hdr * ip6 = malloc(sizeof(struct ip6_hdr));

	// Combine version, traffic class and flow label into the flow field
	ip6->ip6_ctlun.ip6_un1.ip6_un1_flow = htonl((fv->ipv6_version << 7*4) + (fv->ipv6_traffic_class << 6*4) + fv->ipv6_flow_label);
	ip6->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(fv->ipv6_payload_length);
	ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt = fv->ipv6_next_header;
	ip6->ip6_ctlun.ip6_un1.ip6_un1_hlim = fv->ipv6_hop_limit;

	// Copy addresses
	for (int i = 0; i < 8; i++){
		ip6->ip6_src.s6_addr[i] = fv->ipv6_dev_prefix[i];
	}
	for (int i = 8; i < 16; i++){
		ip6->ip6_src.s6_addr[i] = fv->ipv6_dev_iid[i-8];
	}
	for (int i = 0; i < 8; i++){
		ip6->ip6_dst.s6_addr[i] = fv->ipv6_app_prefix[i];
	}
	for (int i = 8; i < 16; i++){
		ip6->ip6_dst.s6_addr[i] = fv->ipv6_app_iid[i-8];
	}

	// Prepare udp header
	struct udphdr * udp = malloc(sizeof(struct udphdr));
	udp->uh_sport = htons(fv->udp_dev_port);
	udp->uh_dport = htons(fv->udp_app_port);
	udp->uh_ulen = htons(fv->udp_length);
	udp->uh_sum = 0;

	size_t payload_len = fv->udp_length - SIZE_UDP;
	// Calculate udp checksum
	// First, we prepare the buffer with the UDP/IPv6 packet
	uint8_t packet[SIZE_IPV6 + SIZE_UDP + fv->udp_length];
	memcpy(packet, ip6, SIZE_IPV6);
	memcpy(packet + SIZE_IPV6, udp, SIZE_UDP);
	memcpy(packet + SIZE_IPV6 + SIZE_UDP, fv->payload, payload_len);

	uint16_t sum = (uint16_t) checksum_udp_ipv6(packet, SIZE_IPV6 + SIZE_UDP + payload_len);

	return sum;

}

void schc_DA_not_sent(struct field_values *fv, enum fieldid fieldid, char *tv) {

	// Variables to convert hex string to byte array
	uint8_t tmp[8];
	uint8_t *pos = tv;

	switch(fieldid) {
		case IPV6_VERSION:
			fv->ipv6_version = atoi(tv);
			break;
		case IPV6_TRAFFIC_CLASS:
			fv->ipv6_traffic_class = atoi(tv);
			break;
		case IPV6_FLOW_LABEL:
			fv->ipv6_flow_label = atoi(tv);
			break;
		case IPV6_NEXT_HEADER:
			fv->ipv6_next_header = atoi(tv);
			break;
		case IPV6_HOP_LIMIT:
			fv->ipv6_hop_limit = atoi(tv);
			break;
		case IPV6_DEV_PREFIX:
			// Copy ipv6 address
			for (int i = 0; i < 8; i++) {
				sscanf(pos, "%2hhx", &tmp[i]);
				pos+=2;
			}	
			memcpy(fv->ipv6_dev_prefix, tmp, 8);
			break;
		case IPV6_DEVIID:
			// Copy ipv6 address
			for (int i = 0; i < 8; i++) {
				sscanf(pos, "%2hhx", &tmp[i]);
				pos+=2;
			}
			memcpy(fv->ipv6_dev_iid, tmp, 8);
			break;
		case IPV6_APP_PREFIX:
			// Copy ipv6 address
			for (int i = 0; i < 8; i++) {
				sscanf(pos, "%2hhx", &tmp[i]);
				pos+=2;
			}
			memcpy(fv->ipv6_app_prefix, tmp, 8);
			break;
		case IPV6_APPIID:
			// Copy ipv6 address
			for (int i = 0; i < 8; i++) {
				sscanf(pos, "%2hhx", &tmp[i]);
				pos+=2;
			}
			memcpy(fv->ipv6_app_iid, tmp, 8);
			break;
		case UDP_DEVPORT:
			fv->udp_dev_port = atoi(tv); 
			break;
		case UDP_APPPORT:
			fv->udp_app_port = atoi(tv); 
			break;
		default:
			break;
	}

}

void schc_DA_value_sent(struct field_values *fv, enum fieldid fieldid, uint8_t *schc_packet, uint8_t *offset) {

	switch(fieldid) {
		case IPV6_VERSION:
			memcpy(&fv->ipv6_version, &schc_packet[*offset], 1);
			*offset+=1;
			break;
		case IPV6_TRAFFIC_CLASS:
			memcpy(&fv->ipv6_traffic_class, &schc_packet[*offset], 1);
			*offset+=1;
			break;
		case IPV6_FLOW_LABEL:
			memcpy(&fv->ipv6_flow_label, &schc_packet[*offset], 3);
			fv->ipv6_flow_label = fv->ipv6_flow_label << 8;
			fv->ipv6_flow_label = ntohl(fv->ipv6_flow_label);
			*offset+=3;
			break;
		case IPV6_PAYLOAD_LENGTH:
			memcpy(&fv->ipv6_payload_length, &schc_packet[*offset], 2);
			fv->ipv6_payload_length = ntohs(fv->ipv6_payload_length);
			*offset+=2;
			break;
		case IPV6_NEXT_HEADER:
			memcpy(&fv->ipv6_next_header, &schc_packet[*offset], 1);
			*offset+=1;
			break;
		case IPV6_HOP_LIMIT:
			memcpy(&fv->ipv6_hop_limit, &schc_packet[*offset], 1);
			*offset+=1;
			break;
		case IPV6_DEV_PREFIX:
			memcpy(fv->ipv6_dev_prefix, &schc_packet[*offset], 8);
			*offset+=8;
			break;
		case IPV6_DEVIID:
			memcpy(fv->ipv6_dev_iid, &schc_packet[*offset], 8);
			*offset+=8;
			break;
		case IPV6_APP_PREFIX:
			memcpy(fv->ipv6_app_prefix, &schc_packet[*offset], 8);
			*offset+=8;
			break;
		case IPV6_APPIID:
			memcpy(fv->ipv6_app_iid, &schc_packet[*offset], 8);
			*offset+=8;
			break;
		case UDP_DEVPORT:
			memcpy(&fv->udp_dev_port, &schc_packet[*offset], 2);
			fv->udp_dev_port = ntohs(fv->udp_dev_port);
			*offset+=2;
			break;
		case UDP_APPPORT:
			memcpy(&fv->udp_app_port, &schc_packet[*offset], 2);
			fv->udp_app_port = ntohs(fv->udp_app_port);
			*offset+=2;
			break;
		case UDP_LENGTH:
			memcpy(&fv->udp_length, &schc_packet[*offset], 2);
			fv->udp_length = ntohs(fv->udp_length);
			*offset+=2;
			break;
		case UDP_CHECKSUM:
			memcpy(&fv->udp_checksum, &schc_packet[*offset], 2);
			fv->udp_checksum = ntohs(fv->udp_checksum);
			*offset+=2;
			break;
		default:
			break;
	}

}

int schc_decompress(uint8_t *schc_packet, uint8_t schc_packet_len, struct field_values *fv) {

	PRINTF("schc_decompress -> decompressing\n");
	PRINTF("schc_packet: ");
	PRINT_ARRAY(schc_packet, schc_packet_len);

	// Extract rule_id used for compression
	uint8_t rule_id = (uint8_t) schc_packet[0];

	/*
	* If the rule does not exists in context.c, we silently drop the
	* packet. {
	*/
	int nrules = sizeof(rules) / sizeof(rules[0]);

	if (rule_id >= nrules)
		return -1;

	/*
	* } If the rule does not exists in context.c, we silently drop the
	* packet.
	*/

	PRINTF("schc_decompress -> using rule_id: %hu\n", rule_id);
	
	// Offset to go through the compression residue
	uint8_t offset = 1;

	// Iterate over the rule
	for (int i = 0; i < 14; i++){

		// Extract rule data
		enum fieldid fieldid = rules[rule_id][i].fieldid;
		size_t field_length = rules[rule_id][i].field_length;
		int field_position = rules[rule_id][i].field_position;
		enum direction direction = rules[rule_id][i].direction;
		enum MO MO = rules[rule_id][i].MO;
		enum CDA CDA = rules[rule_id][i].CDA;

		// Check action
		switch (CDA) {
			case NOT_SENT:
				schc_DA_not_sent(fv, fieldid, rules[rule_id][i].tv);
				break;
			case VALUE_SENT:
				schc_DA_value_sent(fv, fieldid, schc_packet, &offset);
				break;
			case MAPPING_SENT:
				break;
			case LSB:
				break;
			// If length or checksum is needed to be computed, we must do it at the end, as we
			// need all the field values
			case COMPUTE_LENGTH:
				compute_length = 1;
				break;
			case COMPUTE_CHECKSUM:
				compute_checksum = 1;
				break;
			case DEVIID:
				break;
			case APPIID:
				break;
			default:
				break;
		}
	}

	size_t payload_length = 0;

	// Now we can compute the length of the payload
	if (compute_length)	{
		// The length is the schc packet length minus the rule id and the compression residue
		payload_length = schc_packet_len - offset;

		fv->ipv6_payload_length = payload_length + SIZE_UDP;
		fv->udp_length = payload_length  + SIZE_UDP;

		compute_length = 0;
	}

	// Add payload to the struct
	memcpy(fv->payload, &schc_packet[offset], payload_length);

	// Now that we have the payload, we can compute the udp checksum
	if (compute_checksum) {
		uint16_t sum = compute_udp_checksum(fv);
		fv->udp_checksum = sum;

		compute_checksum = 0;
	}

	// Print the reconstructed packet

	// TODO print_udpIp6_packet(fv) assumes always downlink packets
	// (when dev_iid is the ipv6_dst_addr), if we use it to print here,
	// the values will be scrambled.

	// print_udpIp6_packet(fv);

	return 0;

}
