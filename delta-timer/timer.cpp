//#includes
#include "timer.h"
#include "types.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "functions.h"
#include <list>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// From cont
#define TIMER_IN_PORT 6669




static TimerDeltaList deltaList;
static TimerInput** inputList; 
static int* inputListHead;
static int* inputListTail;
static int* inputListCount;
static const int InputListCapacity = 50;


static Error_T SendSeqNoToPort(PortNo_T portNo, SeqNo_T seqNo)
{
    unsigned int outputSock;
    sockaddr_in outputAddr;
    ssize_t size;
    outputSock = socket(AF_INET, SOCK_DGRAM, 0);

    outputAddr.sin_family = AF_INET; // Address family to use
    outputAddr.sin_port = htons(portNo); // Port num to use
    outputAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP address to use

    size = Send(outputSock, (void*)&seqNo, sizeof(SeqNo_T), &outputAddr);
    printf("Sending message to port: %d, for seqNo: %d\n", portNo, seqNo);

	close(outputSock);
    return 0;
}

static unsigned RecvMsgsOnPort()
{
    //Bind to input port
    unsigned int inputSock;
    sockaddr_in inputAddr;

    inputSock = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(inputSock, F_SETFL, O_NONBLOCK);
    
    // Setup struct to receive data from tcpd
    inputAddr.sin_family      = AF_INET;            // Address family to use
    inputAddr.sin_port        = htons(TIMER_IN_PORT);    // Port number to use
    inputAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on any IP address
    bind(inputSock, (struct sockaddr *)&inputAddr, sizeof(inputAddr));
    
    puts("Listening for timer requests.");

    return inputSock;
}


static int checkNewRequests(unsigned inputSock)
{
    TimerInput input;
    TimerInfo info;
    while ( Recv(inputSock, (void*)&info, sizeof(TimerInfo)) != -1 )
    {
        input.timestamp = time(NULL);
        input.info = info;

        if(*inputListCount >= InputListCapacity)
        {
            puts("Dropping request, capacity reached.");
            continue;
        }

        (*inputList)[(*inputListTail)++] = input;
        (*inputListCount)++;
        if (*inputListTail == InputListCapacity)
            (*inputListTail) = 0;
        //printf("Added timer request to queue. Count: %d\n", (*inputListCount));
    }

    return 0;
}

void InitializeGlobalMemory()
{
    inputListHead = new int;
    inputListTail = new int;
    inputListCount = new int;
    (inputList) = new TimerInput*;

    *inputListHead = 0;
    *inputListTail = 0;
    *inputListCount = 0;
    (*inputList) = new TimerInput[InputListCapacity];
}


int main(int argc, char** argv)
{
    UINT32 ticks;
    TimerOutput output;
    Error_T error;

    InitializeGlobalMemory();

    int inputSock = RecvMsgsOnPort();

    while(1)
    {
        //TODO: Implement variable timeout depending on duration of processing.
        usleep(TICK_DURATION_IN_us);
        //Update head on TimerDeltaList

        if (deltaList.Count() > 0)
        {
            error = deltaList.Peek(ticks, output);
            if (!error)
               deltaList.SetHeadPriority(--ticks);
            else
                puts("Error reading delta list.");
        }

        checkNewRequests(inputSock);

        while ((*inputListCount) > 0)
        {

            TimerInput input = (*inputList)[(*inputListHead)];
            TimerOutput output;
            output.port = input.info.port;
            output.seqNo = input.info.seqNo;

            int timeout_ticks = input.info.delayInms;
            if (timeout_ticks <= 0)
            {
                printf("Timeout less than 1ms %d", timeout_ticks);
                exit(1);
            }

            deltaList.Enqueue(timeout_ticks, output);

            (*inputListHead)++;
            (*inputListCount)--;
            if ((*inputListHead) == InputListCapacity)
                (*inputListHead) = 0;

            printf("Delta list count: %u, Input list count: %d\n",
                deltaList.Count(), (*inputListCount));

        }

        //Dequeue logic
        error = deltaList.Peek(ticks, output);
        while (ticks <= 0 && !error)
        {
            error = deltaList.Dequeue(ticks, output);
            //printf("Timeout expired for head. Delta list count: %u\n", deltaList.Count());
            SendSeqNoToPort(output.port, output.seqNo);
            error |= deltaList.Peek(ticks, output);
        }
    }
}
