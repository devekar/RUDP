/*
 * timer-test.cpp
 *
 *  Created on: Mar 6, 2014
 *      Author: vegeta
 */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <sys/time.h>

#include "types.h"
#include "functions.h"


#ifndef __SEND_TO_TIMER_H
#define __SEND_TO_TIMER_H


// From constants.h
#define TIMER_IN_PORT 6669
#define TCPD_TIMER_PORT 7000


static Error_T SendToTimer(int delayInms, SeqNo_T seqNo)
{
    unsigned int outputSock;
    sockaddr_in outputAddr;
    ssize_t size;
    TimerInfo info;

    info.port = TCPD_TIMER_PORT;
    info.delayInms = delayInms;
    info.seqNo = seqNo;

    outputSock = socket(AF_INET, SOCK_DGRAM, 0);

    outputAddr.sin_family = AF_INET; // Address family to use
    outputAddr.sin_port = htons(TIMER_IN_PORT); // Port num to use
    outputAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP address to use

    size = Send(outputSock, &info, sizeof(TimerInfo), &outputAddr);
    //printf("Client: Sent message to timer. Port: %d, SeqNo: %d, delay in ms: %d\n",
    //        TCPD_TIMER_PORT, seqNo, delayInms);

    if (size < 0)
    {
        puts("Error sending data to timer process. Exiting.");
        exit(1);
    }

	close(outputSock);
    return 0;
}

#endif
