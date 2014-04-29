#ifndef __SENDBUFFER_H
#define __SENDBUFFER_H

#include "types.h"
#include "constants.h"
#include "buffer.h"
#include "tcp.h"
#include "delta-timer/send-to-timer.h"

//READY meaning data can be sent, INFLIGT meaning waiting for ACK or timeout so ignore for now, ACKED meaning seq_num has been been acknowledged
#define READY 0
#define INFLIGHT 1
#define ACKED 2

#define MAX_WINDOW 20
#define WINDOW_DS_SIZE (MAX_WINDOW + 1)

//Circular buffer with data added on the tail side, extra slot to be unused and for distinguishing buffer full/empty
class SendBuffer:public Buffer {

seqNo_t window[WINDOW_DS_SIZE];
int window_segment_size[WINDOW_DS_SIZE];
int window_status[WINDOW_DS_SIZE];
int window_size, window_head, window_tail;
seqNo_t next_base_seq_num;
int outside_window_size;        // Keep track of data not yet added to window

public:

SendBuffer()
{
    window_head = 0;
    window_tail = 0;
    window_size = 0;
    outside_window_size = 0;
    next_base_seq_num = 0;
}


int push_back(char *buf, int n)
{
    outside_window_size += n;
    return Buffer::push_back(buf, n);
}


//Get a READY seq_num from current window, also mark it as INFLIGHT
int get_data_and_mark_as_INFLIGHT(char buffer[], seqNo_t *seq_num)
{
    int i, offset, data_size;

    for(i = window_head; i!= window_tail; i = (i+1)%WINDOW_DS_SIZE )
        if(window_status[i] == READY)
            break;

    if(i==window_tail)
        return 0;

    *seq_num = window[i];
    offset = window[i] - window[window_head];
    data_size = window_segment_size[i];
    get_data(buffer, data_size, offset);
    window_status[i] = INFLIGHT;

    return data_size;
}



void print_window()
{
    printf("Window Seqs: ");

    for(int i=window_head; i!=window_tail; i=(i+1)%WINDOW_DS_SIZE )
        printf("%d ", window[i]);

    printf("\n");
}

//Increment window if data available
void increment_window()
{
    if( window_size == MAX_WINDOW ) return;
    int old_tail = window_tail;

    int i;
    for(    i=window_tail;
            window_size < MAX_WINDOW && outside_window_size/MSS;
            i=(i+1)%WINDOW_DS_SIZE, outside_window_size -= MSS, window_size++ )
    {
        window[i] = next_base_seq_num;
        window_segment_size[i] = MSS;
        next_base_seq_num += window_segment_size[i];
        window_status[i] = READY;
    }

    // If there's data less than MSS size
    if(window_size < MAX_WINDOW && outside_window_size%MSS)
    {
        window[i] = next_base_seq_num;
        window_segment_size[i] = outside_window_size%MSS;
        next_base_seq_num += window_segment_size[i];
        window_status[i] = READY;

        outside_window_size = 0;
        i=(i+1)%WINDOW_DS_SIZE;
        window_size++;
    }
    window_tail = i;
    if(window_tail != old_tail)
        printf("Increment, Window size: %d\n", window_size);
}

//Reduce window if the earliest seq_num has been acknowledged
void decrement_window()
{
    if(!window_size) return;
    int i, pop_size=0;

    for(i=window_head; i!=window_tail; i=(i+1)%WINDOW_DS_SIZE )
    {
        if( window_status[i]!=ACKED )
            break;
        else
        {
            window_size--;
            pop_size += window_segment_size[i];
        }
    }

    window_head = i;
    Buffer::pop_front(pop_size);
    printf("Decrement, Window size: %d\n", window_size);
}


//Mark the seq_num in window as acknowledged, also check if window can be decremented
void mark_ack(seqNo_t seq_num)
{
    int i;
    for(i=window_head; i!=window_tail; i=(i+1)%WINDOW_DS_SIZE )
    {
        if( window[i]==seq_num )
        {
            window_status[i]=ACKED;
            printf("ACK %d\n", seq_num);
            break;
        }
    }

    if(i != window_tail)
        decrement_window();
    else
        printf("Discarded ACK %d\n", seq_num);
}

//Mark the seq_num as READY so that it can be sent again
void mark_timeout(seqNo_t seq_num)
{
    for(int i=window_head; i!=window_tail; i=(i+1)%WINDOW_DS_SIZE )
    {
        if( window[i]==seq_num )
        {
            if(window_status[i] != ACKED)
            {
                window_status[i]=READY; printf("Timeout %d\n", seq_num);
                return;
            }
            break;
        }
    }
    printf("Discard timeout %d\n", seq_num);
}

seqNo_t getNextSeqNo()
{
    return next_base_seq_num++;
}

void clear()
{
    window_head = 0;
    window_tail = 0;
    window_size = 0;
    outside_window_size = 0;
    next_base_seq_num = 0;
    Buffer::clear();
}

virtual ~SendBuffer()
{

}

};


#endif
