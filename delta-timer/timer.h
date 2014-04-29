#ifndef _RUDP_TIMER_H_
#define _RUDP_TIMER_H_

#include <unistd.h>
#include <time.h>
#include "types.h"
#include "delta-list.h"


#define TICK_DURATION_IN_us 1000

struct TimerInput
{
    time_t timestamp;
    TimerInfo info;
};

struct TimerOutput
{
    PortNo_T port;
    SeqNo_T seqNo;
};

typedef DeltaList<TimerOutput> TimerDeltaList;

#endif
