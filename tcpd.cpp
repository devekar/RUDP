#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "timeoutCalculator.h"
#include "types.h"
#include "constants.h"
#include "tcp.h"
#include "sendbuffer.h"
#include "recvbuffer.h"
#include "buffer.h"
#include "delta-timer/send-to-timer.h"

static SendBuffer send_buffer;
static RecvBuffer recv_buffer;

static int in_sockfd, out_sockfd, recv_sockfd, timer_sockfd;
static struct sockaddr_in in_addr, out_addr, recv_addr, timer_addr;
static socklen_t in_addr_len = sizeof(in_addr);
static socklen_t recv_addr_len = sizeof(recv_addr);

static bool CLOSING_block = false;
static bool ACCEPT_block = false;
// FTPS wants data, but we don't have any currently
static bool RECV_block = false;
// FTPC send rate is faster than what the TCPD send rate
static bool SEND_block = false;

// How much data did FTPS want?
static int RECV_size = 0;

// Has connection shutdown been initiated?
static bool isClosing = false;

// Has FIN been sent?
static bool finSent = false;

// Has FIN ACK been recvd?
static bool finAck = false;

// Has FIN been received?
static bool finRecv = false;

static seqNo_t finSeqNo = 0;

static TimeoutCalculator tCalculator;

// Buffer for storing data received from user
static char userBuffer[MSS+1];

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// create sockaddr_in for local port and bind it
struct sockaddr_in make_addr_and_bind(int sockfd, int port)
{
    struct sockaddr_in local_addr;

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        printf("Port %d ", port);
        error("binding");
    }

    return local_addr;
}

// create sockaddr_in for target end
struct sockaddr_in make_remote_addr(const char host[], int port)
{
    struct hostent *hp;// *gethostbyname();
    struct sockaddr_in remote_addr;

    /* construct name for connecting to server */
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);

    /* convert hostname to IP address and enter into name */
    hp = gethostbyname(host);
    if(hp == 0)
        error("unknown host\n");
    bcopy((char *)hp->h_addr, (char *)&remote_addr.sin_addr, hp->h_length);

    return remote_addr;
}

//Fetch input from user. recvBytes is set if new data is fetched.
void getInputFromUser(char &controlByte, int& recvBytes)
{
    if(ACCEPT_block)
        controlByte = 'A';
    else if(RECV_block)
        controlByte = 'R';
    else if(SEND_block)
        controlByte = 'S';
    else
    {
        recvBytes = recvfrom(
                in_sockfd,
                userBuffer,
                MSS + 1,
                0,
                (struct sockaddr *)&in_addr,
                &in_addr_len);

        controlByte = userBuffer[0];
    }
}

//receive from user
//check control byte if send/recv
//if send, add to send buffer, unblock FTPC
//if recv, fetch from buffer and send to FTPS or if empty, RECV_block ensures retrial again on next call
//recv_bytes includes control_byte
void user_action(char control_byte, int recv_bytes)
{
    if(recv_bytes < 0) return;

    int sent_bytes, n;

    //If user has initiated connection shutdown, ignore all future user actions.
    if (CLOSING_block)
        return;

    // Take appropriate action based on control byte
    switch(control_byte)
    {
        case 'X' :  // Test communication by sending back same data
            sent_bytes = sendto(
                    in_sockfd,
                    userBuffer,
                    recv_bytes,
                    0,
                    (struct sockaddr *)&in_addr,
                    in_addr_len);
            if(sent_bytes < 0)
                error("SOCKET:IPC failed\n");
            else
                printf("SOCKET: IPC working\n");
            break;

        case 'S':
            SEND_block = !send_buffer.hasCapacity(recv_bytes - 1);
            if (SEND_block)
            {
                puts("Waiting for send window to proceed before accepting more data from user");
                break;
            }
            n = send_buffer.push_back(userBuffer +1, recv_bytes -1);
            if(n==-1)
                error("Buffer Overflow");

            // Unblock sender by sending something
            sent_bytes = sendto(
                    in_sockfd,
                    userBuffer,
                    MSS,
                    0,
                    (struct sockaddr *)&in_addr,
                    in_addr_len);
            break;

        case 'R':  // User wants data from recv_buffer
            if(!RECV_block)
            {
                memcpy(&RECV_size, userBuffer+1, sizeof(RECV_size));
                RECV_block = true;
            }

            n = recv_buffer.get_front(userBuffer, RECV_size);
            if(n < 0)
                break;

            // Send data to user
            sent_bytes = sendto(in_sockfd, userBuffer, n, 0, (struct sockaddr *)&in_addr, in_addr_len);
            if(sent_bytes <= 0)
                break;

            recv_buffer.pop_front(n);
            RECV_block = false;
            break;

        case 'A':    //If any data in recv_buffer, unblock FTPS
            ACCEPT_block = true;
            if(recv_buffer.size()) {
                userBuffer[0] = 'A';
                sent_bytes = sendto(
                        in_sockfd,
                        userBuffer,
                        1,
                        0,
                        (struct sockaddr *)&in_addr,
                        in_addr_len);
                if(sent_bytes < 0)
                    error("ACCEPT:IPC failed\n");

                printf("ACCEPT call\n");
                ACCEPT_block = false;
            }
            break;


        case 'C':
            CLOSING_block = isClosing = true;
            break;

        case 'B': //BIND
            break;
        case 'L': //LISTEN
            break;
        case 'Y': // CONNECT
            break;
        default:  //Unknown control byte
            printf("Invalid message: %c  %.*s\n", userBuffer[0], recv_bytes -1 , userBuffer + 1);
            break;
    }
}

//Increment window if possible and get data from window to be sent, start timer
void send_data()
{
    if(!send_buffer.size())
        return;
    send_buffer.increment_window();

    int sent_bytes, sent_packets=0;
    seqNo_t seq_num;
    char buffer[MTU];

    while(true)
    {
        int len = send_buffer.get_data_and_mark_as_INFLIGHT(buffer, &seq_num);

        if (len==0) break;

        len = make_data_packet(buffer, len, seq_num);
        sent_bytes = sendto(
                out_sockfd,
                buffer,
                len,
                0,
                (struct sockaddr *)&out_addr,
                sizeof(out_addr) );
        tCalculator.SentSeq(seq_num);
        printf("Sent SEQ:%d\n", seq_num);
        SendToTimer(tCalculator.GetTimeoutInms(), seq_num);
        sent_packets++;
    }

    if(sent_packets)
        printf("Sent packets %d\n", sent_packets);
}

//Construct ACK packet and send
void send_ack(seqNo_t seq_num)
{
    char buffer[MTU];
    int len = make_ack_packet(buffer, seq_num);
    int sent_bytes = sendto(
            out_sockfd,
            buffer,
            len,
            0,
            (struct sockaddr *)&out_addr,
            sizeof(out_addr) );
    printf("Sent ACK: %d\n", seq_num);
}

//net_buffer must be of size at least MTU
int recv_data_from_connection(char* net_buffer)
{
    int recv_bytes = recvfrom(
            recv_sockfd,
            net_buffer,
            MTU,
            0,
            (struct sockaddr *)&recv_addr,
            &recv_addr_len);

    if(recv_bytes == -1)
        return -1;

    // Verify checksum of packet
    if(!verify_checksum(net_buffer, recv_bytes))
    {
        printf("Packet Garbled, discard\n");
        return -1;
    }

    return recv_bytes;
}

/*
 * If data, send ACK and put in recv_buffer; if ACK, mark it in send_buffer
 * window.
 */
void recv_data_acks()
{
    bool isAck, isFin;
    seqNo_t seq_num, ack_num;
    int recv_bytes;
    char net_buffer[MTU];

    recv_bytes = recv_data_from_connection(net_buffer);

    if (recv_bytes < 0)
        return;

    isAck = isACK(net_buffer, recv_bytes);
    isFin = isFIN(net_buffer, recv_bytes);

    if (isAck)
    {
        getAckNo(net_buffer, recv_bytes, ack_num);
        tCalculator.RecvAck(ack_num);
        printf("Timeout updated to: %d\n", tCalculator.GetTimeoutInms());

        if (finSent && finSeqNo == ack_num)
        {
            printf("FIN ack received. AckNo: %u\n", ack_num);
            finAck = true;
        }
        else
            send_buffer.mark_ack(ack_num);
    }
    else
    {
        getSeqNo(net_buffer, recv_bytes, seq_num);
        if (isFin)
        {
            printf("FIN received. SeqNo: %u\n", seq_num);
            finRecv = isClosing = true;
        }
        else
        {
            int len = extract_data_from_packet(net_buffer, recv_bytes, &seq_num);
            recv_buffer.put_data(net_buffer, seq_num, len);
        }
        send_ack(seq_num);
    }
}

//Check if timeouts and mark it in send_buffer window, so that these packets can be sent again
void recv_timeouts()
{
    seqNo_t timer_seq_num;
    int recv_bytes = Recv(timer_sockfd, (void*)&timer_seq_num, sizeof(seqNo_t));
    if(recv_bytes > 0 )
        send_buffer.mark_timeout(timer_seq_num);
}

/*
 * Send data / Receive data over connection for user until shutdown is initiated.
 * Sets isActive = false before returning.
 */

void executeShutdown()
{
    if (finSent)
        error("Invalid program state.");

    //If FIN has already been received, this node is passive closer and vice versa
    bool activeCloser = !finRecv;

    printf("Is node active closer? %d\n", activeCloser);
    int sent_bytes, len, errorCode, retryCount = 0;
    char buffer[MTU];
    seqNo_t seqNo = send_buffer.getNextSeqNo();
    len = make_fin_packet(buffer, seqNo);

    while (!finAck && retryCount < TCP_FIN_MAX_RETRY)
    {
        sent_bytes = sendto(
                        out_sockfd,
                        buffer,
                        len,
                        0,
                        (struct sockaddr *)&out_addr,
                        sizeof(out_addr) );
        if (sent_bytes < 0)
            error("Error sending FIN packet.");
        printf("FIN sent. SeqNo: %u\n", seqNo);        
        finSeqNo = seqNo;
        finSent = true;
        retryCount++;
        SendToTimer(tCalculator.GetTimeoutInms(), seqNo);

        while (!finAck)
        {
            recv_data_acks();
            
            seqNo_t timer_seq_num;
            int recv_bytes = Recv(
                    timer_sockfd,
                    (void*)&timer_seq_num,
                    sizeof(seqNo_t));
            if (recv_bytes > 0 && timer_seq_num == finSeqNo)
            {
                puts("FIN timeout expired");
                break;
            }
        } 
    }

    //Wait for FIN recv
    if (!finRecv)
    {   
        puts("Waiting for receiving FIN");
        while (!finRecv)
            recv_data_acks();
        puts("FIN received. Proceeding with connection shutdown.");
    }

    //Active closer needs to wait for ACK for peer FIN to get delivered
    if (activeCloser)
    {
        puts("Active closer, hence waiting to acknowledge any redundant FINs");
        seqNo_t timer_seq_num;
        int recv_bytes;
        seqNo = send_buffer.getNextSeqNo();
        SendToTimer(TCP_MSL_IN_MS * 2, seqNo);
        while(true)
        {
            recv_data_acks();
            recv_bytes = Recv(timer_sockfd, (void*)&timer_seq_num, sizeof(seqNo_t));
            if(recv_bytes > 0 && timer_seq_num == seqNo)
                break;
        }
    }

    //Send random data to unblock client.
       sent_bytes = sendto(
               in_sockfd,
               userBuffer,
               1,
               0,
               (struct sockaddr *)&in_addr,
               in_addr_len);

    if (finAck)
        puts("Connection shutdown successful");
    else if (retryCount >= TCP_FIN_MAX_RETRY && !finAck)
        puts("FIN packet retransmits failed. Terminating connection");
    else
        puts("Connection closed. Errors were encountered");
}

void executeConnection(int recv_bytes)
{

    //TODO: select/poll might be more efficient than a continuous loop      <-------------------------------------------
    //isActive is set to false when the connection has shutdown
    char controlByte;

    printf("Start controlByte: %c\n", userBuffer[0]);

    if (userBuffer[0] == 'C') //connection close
    {
        puts("Error: Connection start ignored. User command: 'C'");
        return;
    }

    //Some data was already fetched from user while waiting for start
    user_action(userBuffer[0], recv_bytes);

    while(!(isClosing && send_buffer.size() == 0))
    {
        send_data();
        recv_data_acks();
        recv_timeouts();
        getInputFromUser(controlByte, recv_bytes);
        user_action(controlByte, recv_bytes);
    }

    executeShutdown();
}

//Resets connection state variables
void refreshConnection()
{
    puts("Refreshing connection parameters");
    ACCEPT_block = RECV_block = CLOSING_block = SEND_block = false;
    RECV_size = 0;
    isClosing = finSent = finAck = finRecv = false;
    finSeqNo = 0;

    send_buffer.clear();
    recv_buffer.clear();
    tCalculator.Clear();
}

//Setup sockets for user process, timer, sending data to troll and receiving data.
static void setupSockets()
{
    /*create socket for IPC with user process and bind it*/
    in_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(in_sockfd < 0)
        error("opening datagram socket");
    fcntl(in_sockfd, F_SETFL, O_NONBLOCK);                    // NON-BLOCKING
    in_addr = make_addr_and_bind(in_sockfd, TCPD_IN_PORT);

    /*create socket for IPC with timer process and bind it*/
    timer_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(timer_sockfd < 0)
        error("opening datagram socket");
    fcntl(timer_sockfd, F_SETFL, O_NONBLOCK);                    // NON-BLOCKING
    timer_addr = make_addr_and_bind(timer_sockfd, TCPD_TIMER_PORT);
    //timer_addr = make_remote_addr(LOCALHOST, TIMER_IN_PORT);

    /*create socket for troll communication and bind it*/
    out_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(out_sockfd < 0)
        error("opening datagram socket");
    out_addr = make_addr_and_bind(out_sockfd, TCPD_OUT_PORT);

    //reuse out_addr to define target end i.e. troll
    out_addr = make_remote_addr(LOCALHOST, TROLL_IN_PORT);

    /*create socket for receiving communication and bind it*/
    recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(recv_sockfd < 0)
        error("opening datagram socket");
    fcntl(recv_sockfd, F_SETFL, O_NONBLOCK);                    // NON-BLOCKING
    recv_addr = make_addr_and_bind(recv_sockfd, TCPD_RECV_PORT);
}

//Close sockets for user process, timer, sending data to troll and receiving data.
static void closeSockets()
{
    close(in_sockfd);
    close(out_sockfd);
    close(recv_sockfd);
    close(timer_sockfd);
}

int main(int argc, char *argv[])
{
    while (true)
    {
        refreshConnection();
        setupSockets();
        int recv_bytes = 0;
        //Wait for user to initiate data transfer
        puts("Waiting for clients to initiate connection");
        while (recv_bytes <= 0)
        {
            sleep(1);
            recv_bytes = recvfrom(
                            in_sockfd,
                            userBuffer,
                            MSS + 1,
                            0,
                            (struct sockaddr *)&in_addr,
                            &in_addr_len);
        }
        executeConnection(recv_bytes);
        closeSockets();
    }
    return 0;
}
