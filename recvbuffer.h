#ifndef __RECVBUFFER_H
#define __RECVBUFFER_H

#include <set>
#include <utility>

#include "types.h"
#include "constants.h"
#include "buffer.h"
#include "tcp.h"

using namespace std;


#define MAX_WINDOW 20
#define WINDOW_DS_SIZE (MAX_WINDOW + 1)

//RecvBuffer implements window indirectly via a set of pairs, a pair being sequence number of packet and data size. Window data is buffered in receive buffer but does not account to size of buffer
//set will intrinsically order by sequence number, if we receive lowest sequence number in window, we can advance by removing lowest pairs and inflate the size of receive buffer
//Since send window is 20, we do not need to worry about size of receive window
class RecvBuffer:public Buffer
{
set<pair<int, int> > window;
set<pair<int, int> >::iterator it;
seqNo_t window_head, window_tail;

public:
RecvBuffer()
{
    window_head = 0;
    window_tail = MAX_WINDOW*MSS + 1;
}

//Insert the data into correct position in window
int put_data(char *buf, seqNo_t seq_num, int n)
{
    if(seq_num < window_head || seq_num+n >= window_tail)
        return 0;

    std::pair<std::set<pair<int, int> >::iterator,bool> ret;
    ret = window.insert(pair<int, int>(seq_num, n) );
    if(ret.second==false)
        return 0;

    //Add data directly into buffer of base class
    int offset = seq_num-window_head;
    int bytes = insert(buf, n, offset);

    if(bytes != n)
        return -1;

    if(seq_num==window_head)
        slide_window();

    return n;
}


//If consecutive packets from the older end of window present, move window forward and let recv_buffer claim the data
void slide_window()
{
    int data_size=0, slots = 0;
    for(it = window.begin() ; it != window.end(); it++ )
    {
        if( it->first != window_head ) break;
        window_head = window_head + it->second;
        data_size += it->second;
        slots++;
    }

    int new_size = Buffer::size() + data_size;
    int tail_increment = (MAX_BUFFER - new_size) < 20*MSS ?
            (MAX_BUFFER - new_size):
            20*MSS;
    window_tail = window_tail + tail_increment;

    window.erase(window.begin(), it);
    Buffer::inflate(data_size);

    printf("Window slide by %d\n", slots);
}

void clear()
{
    window_head = 0;
    window_tail = MAX_WINDOW*MSS + 1;
    Buffer::clear();
}

virtual ~RecvBuffer()
{

}
};


#endif
