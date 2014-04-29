#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define MSS 1000
#define TCP_HEADER 20
#define MTU (MSS + TCP_HEADER)
#define TCPD_IN_PORT 5554
#define TCPD_OUT_PORT 5556
#define TROLL_IN_PORT 5557
#define TCPD_RECV_PORT 5558
#define TIMER_IN_PORT 6669
#define TCPD_TIMER_PORT 7000
#define LOCALHOST "localhost"
#define TCP_FIN_MAX_RETRY 10
#define TCP_MSL_IN_MS 60000
#define TCP_RTO_LOWER 1000
#define TCP_RTO_UPPER 60000

//Max length of buffer
#define MAX_BUFFER 65536


#endif
