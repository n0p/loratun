#include <string.h>

#include "schc_fields.h"

#define BUFFER_SIZE 2048

// convert a IPv6 packet to the struct field_values used by schc_compress
int buffer2fields(uint8_t *buffer, int *len, struct field_values *p) {

	if ((buffer[0] & 0xF0) != 0x60) return 1; // no IPv6 packet
	if (*len > BUFFER_SIZE) return 2; // prevent buffer overrun

	int payload_length=*len - 0x30;

	p->ipv6_version      =6;
	p->ipv6_traffic_class=(uint16_t)buffer[0x01]>>4;
	p->ipv6_flow_label   =((buffer[0x01] & 0x0F)<<16) + (buffer[0x02]<<8) + buffer[0x03];
	p->ipv6_next_header  =(uint8_t)buffer[0x06]; // 17 for UDP
	p->ipv6_hop_limit    =(uint8_t)buffer[0x07];
	p->udp_dev_port      =(uint8_t)buffer[0x28]*256 + (uint8_t)buffer[0x29];
	p->udp_app_port      =(uint8_t)buffer[0x2A]*256 + (uint8_t)buffer[0x2B];
	p->udp_length        =(uint8_t)buffer[0x2C]*256 + (uint8_t)buffer[0x2D];
	p->udp_checksum      =(uint8_t)buffer[0x2E]*256 + (uint8_t)buffer[0x2F];
	memcpy(&(p->ipv6_dev_prefix), &buffer[0x08], 32); // copy IPv6 src+dst address
	memcpy(&(p->payload),         &buffer[0x30], payload_length);

	/*
	printf("*ipv6/udp:p class=%x flow=%x next=%x hop=%x udp(sport=%d dport=%d len=%d checksum=%d)\n",
		p->ipv6_traffic_class, p->ipv6_flow_label,
		p->ipv6_next_header, p->ipv6_hop_limit,
		p->udp_dev_port, p->udp_app_port,
		(unsigned int)p->udp_length, (unsigned int)p->udp_checksum
	);
	debug((unsigned char *)p, offsetof(typeof(*p), payload) + payload_length);
	*/

	return 0;

}

// convert the struct field_values passed by schc_decompress to a IPv6 packet
int fields2buffer(struct field_values *p, uint8_t *buffer, int *len) {

	*len=(int)p->udp_length + 0x28;
	if (*len > BUFFER_SIZE) return 1; // prevent buffer overflow
	//memset(buffer, 0, *len); // not needed because all bytes are overwritten
	buffer[0x00]=0x60;
	buffer[0x01]=(p->ipv6_traffic_class*0x10) + ((p->ipv6_flow_label & 0xF0000)>>16);
	buffer[0x02]=(p->ipv6_flow_label & 0xFF00)>>8;
	buffer[0x03]=(p->ipv6_flow_label & 0xFF);
	buffer[0x04]=(p->ipv6_payload_length & 0xFF00)>>8;
	buffer[0x05]=(p->ipv6_payload_length & 0xFF);
	buffer[0x06]=p->ipv6_next_header;
	buffer[0x07]=p->ipv6_hop_limit;
	memcpy(&buffer[0x08], &(p->ipv6_dev_prefix), 32); // copy IPv6 src+dst address
	buffer[0x28]=(p->udp_dev_port & 0xFF00)>>8;
	buffer[0x29]=(p->udp_dev_port & 0xFF);
	buffer[0x2A]=(p->udp_app_port & 0xFF00)>>8;
	buffer[0x2B]=(p->udp_app_port & 0xFF);
	buffer[0x2C]=(p->udp_length & 0xFF00)>>8;
	buffer[0x2D]=(p->udp_length & 0xFF);
	buffer[0x2E]=(p->udp_checksum & 0xFF00)>>8;
	buffer[0x2F]=(p->udp_checksum & 0xFF);
	if ((0x30 + p->ipv6_payload_length - SIZE_UDP) > BUFFER_SIZE) return 2; // prevent buffer overflow
	memcpy(&buffer[0x30], &(p->payload), p->ipv6_payload_length - SIZE_UDP);

	return 0;

}
