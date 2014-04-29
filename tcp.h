#include<stdint.h>

#ifndef __TCP_H
#define __TCP_H

#include "types.h"

typedef struct tcp 
{
	uint16_t src_port;		// Source port
	uint16_t dst_port;		// Destination port
	seqNo_t seq_num;	// Sequence number
	seqNo_t ack_num;	// ACK number

	uint8_t offset:4;	// Offset (bit 0..3)
	uint8_t reserved:3;	// Reserved (bit 4..6)
	uint8_t ns:1;		// NS flag
	uint8_t cwr:1;		// CWR flag
	uint8_t ece:1;		// ECE flag
	uint8_t urg:1;		// URG flag
	uint8_t ack:1;		// ACK flag
	uint8_t psh:1;		// PSH flag
	uint8_t rst:1;		// RST flag
	uint8_t syn:1;		// SYN flag
	uint8_t fin:1;		// FIN flag
	
	uint16_t win_size;	// Window size
	uint16_t chk;		// TCP checksum
	uint16_t urg_ptr;	// Urgent pointer
	
	// No Options field
    //char data[];			//TCP payload (flexible size)
} __attribute__ ((packed)) tcp_t;



uint16_t calculate_checksum(void *data, int len);
void calculate_and_set_checksum(void* packet, int len);
int verify_checksum(void *packet, int len);

void set_fields_for_send(tcp_t* header, uint16_t src_port, uint16_t dst_port, uint32_t seq_num);
void set_fields_for_ack(tcp_t* header, uint16_t src_port, uint16_t dst_port, uint32_t ack_num);
bool isACK(void *packet, int size);
bool isFIN(void *packet, int size);
int getAckNo(void *packet, int size, seqNo_t &ackNo);
int getSeqNo(void *packet, int size, seqNo_t& seqNo);

int make_data_packet(char buffer[], int size, seqNo_t seq_num);
int make_ack_packet(char buffer[], seqNo_t seq_num);
int extract_data_from_packet(char buffer[], int recv_bytes, seqNo_t *seq_num);
int make_fin_packet(char buffer[], seqNo_t seq_num);


#endif
