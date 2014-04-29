#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"



class Buffer
{
//TODO: extra 1 byte not used, test if code works on removing +1
char buffer[MAX_BUFFER+1];	// Extra element to distinguish empty/full scenario
int buffer_head, buffer_tail, buffer_size;

public:

Buffer()
{
    buffer_head = 0;
    buffer_tail = 0;
    buffer_size = 0;
}

int size()
{
    return buffer_size;
}


// Sending buffer: add data from FTPC to buffer for sending
int push_back(char *buf, int n)
{
    if( buffer_size+n > MAX_BUFFER) {printf("Buffer Overflow\n");return -1;}   // Buffer overflow

    int n1 = (buffer_tail + n <= MAX_BUFFER)? n: MAX_BUFFER - buffer_tail;  //TODO: check
    memcpy(buffer + buffer_tail, buf, n1);
    int n2= n - n1;
    memcpy(buffer, buf + n1, n2);

    buffer_tail = (buffer_tail + n)%MAX_BUFFER;
    buffer_size += n;

    printf("Push, Buffer size: %d\n", size());
    return n;
}


//Receiving buffer: slide window forward and claim the data left into buffer
int inflate(int n)
{
    if(buffer_size + n > MAX_BUFFER) {printf("Inflate failed, Buffer Overflow\n");return -1;}   // Buffer overflow
    buffer_size += n;

    buffer_tail = (buffer_tail + n)%MAX_BUFFER;
    return n;
}



// Sending buffer: remove data since it is ACKed
// Receiving buffer: sent data to user so discard it
int pop_front(int n)
{
    if(n>buffer_size) {printf("Buffer Underflow\n"); return -1;}
    buffer_head = (buffer_head+n)%MAX_BUFFER;
    buffer_size -= n;

    printf("Pop, Buffer size: %d\n", size());
    return n;
}


// Receiving buffer: Insert data from the current receiving window, offset from buffer_head
int insert(char *buf, int n, int offset=0)
{
    if(buffer_size + offset + n > MAX_BUFFER) {printf("Insert failed, Buffer Overflow\n");return -1;}   // Buffer overflow

    int start = (buffer_tail + offset)%MAX_BUFFER;
    int n1 = (start + n <= MAX_BUFFER)? n: MAX_BUFFER - start;  //TODO: check
    memcpy(buffer + start, buf, n1);
    int n2= n - n1;
    memcpy(buffer, buf + n1, n2);

    return n;
}


//Sending buffer: get data for a particular slot of window
int get_data(char buf[], int n, int offset)
{
    if(offset+n > buffer_size) {printf("Buffer Underflow\n"); exit(1);}
    int start = (buffer_head + offset)%MAX_BUFFER;

    int n1 = (start + n <= MAX_BUFFER)? n:MAX_BUFFER-start;
    memcpy(buf, buffer+start, n1);

    int n2= n-n1;
    memcpy(buf+n1, buffer, n2);

    return  n;
}

//TODO: check if this function can be replaced by get_data() with default offset=0
// Get data to send to user
int get_front(char *buf, int n)
{
    if(buffer_size==0) return -1;
    int n1 = buffer_size < n? buffer_size: n;

    int n2 = (buffer_head + n1 <= MAX_BUFFER)? n1: MAX_BUFFER - buffer_head;
    memcpy(buf, buffer + buffer_head, n2);

    int n3 = n1 - n2;
    memcpy(buf + n2, buffer, n3);

    return n1;
}

virtual void clear()
{
    buffer_head = 0;
    buffer_tail = 0;
    buffer_size = 0;
}

virtual bool hasCapacity(int nextDataSize)
{
    return MAX_BUFFER >= buffer_size + nextDataSize;
}

virtual ~Buffer()
{
    
}

};


#endif
