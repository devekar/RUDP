#ifndef _RUDP_TYPES_H_
#define _RUDP_TYPES_H_

#include <stdint.h>

typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef int SeqNo_T;
typedef int PortNo_T;
typedef int Error_T;

struct TimerInfo
{
    PortNo_T port;
    SeqNo_T seqNo;
    int delayInms;
};

#endif
