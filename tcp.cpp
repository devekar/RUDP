#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<netinet/in.h>

#include "tcp.h"
#define POLYNOMIAL 0x1021  //Truncated polynomial, CRC-16-CCITT


uint16_t calculate_checksum(void *data, int len)
{
    int len16 = len/2;
    uint16_t  *remainder=(uint16_t*)calloc( len16 + 1 + 1, sizeof(uint16_t) ); //padding for crc width as well as incase of odd number of bytes
    memcpy(remainder, data, len);
    
    uint8_t c; int k;
    uint8_t *str = (uint8_t*)remainder;
    for(k=0;k<len;k+=2) {
        c = str[k];
        str[k] = str[k+1];
        str[k+1] = c;
    }
    
    uint32_t poly = 0;
    uint16_t *second = (uint16_t*)&poly;
    uint16_t *first = second + 1;

    int i,j;
    for (i = 0; i< len16; i++)
    {
        *first = POLYNOMIAL;
        *second = 0;
    
        for(j = 15; j >= 0; j--) {
            poly >>= 1;
            if (remainder[i] & 1<<j) {
                remainder[i] ^= *first;
                remainder[i + 1] ^= *second;
            }
        }
    }
    
    
    if(len%2==0) return remainder[len16];
    
    *first = POLYNOMIAL;
    *second = 0;
    
    for(j = 15; j >= 8; j--) {
        poly >>= 1;
        if (remainder[len16] & 1<<j) {
            remainder[len16] ^= *first;
            remainder[len16 + 1] ^= *second;
        }
    }
    
    //printf("Rem:  %"PRIx16" %"PRIx16"\n", remainder[len16], remainder[len16 + 1]);
    str = (uint8_t*)(remainder + len16);
    str[1] =str[0];
    str[0] = str[3];
    
    return remainder[len16];
}  



void calculate_and_set_checksum(void* packet, int len)
{ 
    tcp_t* header = (tcp_t*)packet;
    uint16_t chk = calculate_checksum(packet, len);
    header->chk = htons(chk); 
}



int verify_checksum(void *packet, int len)
{
    tcp_t* header = (tcp_t*)packet;
    uint16_t original_chk = ntohs(header->chk);
    memset(&header->chk, 0, 2);
    uint16_t chk = calculate_checksum(packet, len);
    if(original_chk == chk) return 1;
    else return 0;
}




void set_fields_for_send(tcp_t* header, uint16_t src_port, uint16_t dst_port, seqNo_t seq_num)
{
    memset(header, 0, sizeof(tcp_t));
    header->src_port = htons(src_port);
    header->dst_port = htons(dst_port);
    header->offset = 5;
    header->seq_num = htonl(seq_num);
}

void set_fields_for_ack(tcp_t* header, uint16_t src_port, uint16_t dst_port, seqNo_t ack_num)
{
    memset(header, 0, sizeof(tcp_t));
    header->src_port = htons(src_port);
    header->dst_port = htons(dst_port);
    header->offset = 5;                    //TODO: no data in our ack packet, so value 5 or 0?
    header->ack = 1;
    header->ack_num = htonl(ack_num);
}


bool isACK(void *packet, int size)
{
    tcp_t* header = (tcp_t*)packet;
    return (header->ack == 1 && size >= sizeof(tcp_t));
}

bool isFIN(void *packet, int size)
{
    tcp_t* header = (tcp_t*)packet;
    return (header->fin == 1 && size >= sizeof(tcp_t));
}

//returns -1 if packet is invalid else 0.
int getAckNo(void *packet, int size, seqNo_t &ackNo)
{
    if (size < sizeof(tcp_t))
        return -1;

    tcp_t* header = (tcp_t*)packet;
    ackNo = ntohl(header->ack_num);
    return 0;
}


//returns -1 if packet is invalid else 0.
int getSeqNo(void *packet, int size, seqNo_t &seqNo)
{
    if (size < sizeof(tcp_t))
        return -1;

    tcp_t* header = (tcp_t*)packet;
    seqNo = ntohl(header->seq_num);
    return 0;
}


// Send an Data packet with header
int make_data_packet(char buffer[], int size, seqNo_t seq_num)
{
    tcp_t header;
    
    set_fields_for_send(&header, 0, 0, seq_num);               //Dummy ports
    memmove(buffer + sizeof(tcp_t), buffer, size);             //Copy data
    memcpy(buffer, &header, sizeof(tcp_t));                    //Copy header
    int len = sizeof(tcp_t) + size;
    calculate_and_set_checksum(buffer, len);                   //Set checksum
                        
    return len;
}                     

// Send ACK packet 
int make_ack_packet(char buffer[], seqNo_t seq_num)
{
    tcp_t header;
    set_fields_for_ack(&header, 0, 0, seq_num);                //Dummy ports
    memcpy(buffer, &header, sizeof(tcp_t));                    //Copy header
    calculate_and_set_checksum(buffer, sizeof(tcp_t));         //Set checksum
    return sizeof(tcp_t);
}


int extract_data_from_packet(char buffer[], int recv_bytes, seqNo_t *seq_num)
{
    tcp_t *header = (tcp_t*)buffer;
    *seq_num = ntohl(header->seq_num);
    int len = recv_bytes - sizeof(tcp_t);
    memmove(buffer, buffer+sizeof(tcp_t), len);
    return len;
}

int make_fin_packet(char buffer[], seqNo_t seq_num)
{
    tcp_t header;
    memset(&header, 0, sizeof(tcp_t));
    header.seq_num = htonl(seq_num);
    header.fin = 1;
    memcpy(buffer, &header, sizeof(tcp_t));                    //Copy header
    calculate_and_set_checksum(buffer, sizeof(tcp_t));         //Set checksum
    return sizeof(tcp_t);
}
