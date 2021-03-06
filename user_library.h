#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "constants.h"

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

struct sockaddr_in tcpd_addr;
int tcpd_addr_len, tcpd_sockfd;

// create sockaddr_in for target end i.e. TCPD
struct sockaddr_in make_tcpd_addr()
{
	struct hostent *hp, *gethostbyname();
	struct sockaddr_in tcpd_addr;
	
	/* construct name for connecting to server */
	tcpd_addr.sin_family = AF_INET;
	tcpd_addr.sin_port = htons(TCPD_IN_PORT);

	/* convert hostname to IP address and enter into name */
	hp = gethostbyname("localhost");
	if(hp == 0) error("unknown host\n");
	bcopy((char *)hp->h_addr, (char *)&tcpd_addr.sin_addr, hp->h_length);
	
	return tcpd_addr;
}






int SOCKET(int family, int type, int protocol)
{
	tcpd_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	tcpd_addr = make_tcpd_addr();
	tcpd_addr_len = sizeof(tcpd_addr);
	
	//Try connection with TCPD
	char buffer[MSS] = "X";
	int buflen = 1;
    if( sendto(tcpd_sockfd, buffer, buflen, 0, (struct sockaddr *)&tcpd_addr, tcpd_addr_len) < 0) error("sending SOCKET message");
    if( recvfrom(tcpd_sockfd, buffer, MSS, 0, (struct sockaddr *)&tcpd_addr, &tcpd_addr_len) < 0) error("receiving SOCKET reply"); 
    if (buffer[0] != 'X') error("Connection issue with TCPD");
	
	return tcpd_sockfd; //return the socket descriptor of local connection with TCPD
}


// Inform TCPD that it is expecting data and then get the data from TCPD
int RECV(int sockfd, void *buf, int len, int flags)
{
	char buffer[MSS] = "R";
	memcpy(buffer+1, &len, sizeof(len));
	if( sendto(tcpd_sockfd, buffer, 1 + sizeof(len), flags, (struct sockaddr *)&tcpd_addr, tcpd_addr_len) < 1 ) error("Sending RECEIVE message\n");
	return recvfrom(tcpd_sockfd, buf, len, flags, (struct sockaddr *)&tcpd_addr, &tcpd_addr_len);
}


// Simply send data following the control byte
int SEND(int sockfd, const void *msg, int len, int flags)
{
	usleep(50*1000);   // Helps prevent buffer overrun 
	char buffer[MSS + 1] = "S";
	memcpy(buffer + 1, msg, len);
	int sent_bytes = sendto(tcpd_sockfd, buffer, len + 1, flags, (struct sockaddr *)&tcpd_addr, tcpd_addr_len);
	int recv_bytes = recvfrom(tcpd_sockfd, buffer, len, flags, (struct sockaddr *)&tcpd_addr, &tcpd_addr_len);  //TCPD received ACK, unblock
	return recv_bytes; // Since it includes the extra control byte
}

//Send close notification to TCPD so that connection termination can be intiated
int CLOSE( int sockfd )
{ 
	if(sockfd != tcpd_sockfd) return 0;
	
    puts("CLOSE call");
	char buffer[MSS] = "C";
	int buflen = 1;
    if( sendto(tcpd_sockfd, buffer, buflen, 0, (struct sockaddr *)&tcpd_addr, tcpd_addr_len) < 0) error("sending CLOSE message");
    
    //TODO: get notification from TCPD that termination has been completed
    if( recvfrom(tcpd_sockfd, buffer, MSS, 0, (struct sockaddr *)&tcpd_addr, &tcpd_addr_len) < 0)
        error("receiving CLOSE reply");

    puts("Connection closed.");

	return close(tcpd_sockfd); 
}


// Return a standard file descriptor such as stdout to distinguish it from one allocated to tcpd_sockfd 
int ACCEPT(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)
{ 
	char buffer[MSS] = "A";
	int buflen = 1;
    if( sendto(tcpd_sockfd, buffer, buflen, 0, (struct sockaddr *)&tcpd_addr, tcpd_addr_len) < 0) error("sending ACCEPT message");
    if( recvfrom(tcpd_sockfd, buffer, MSS, 0, (struct sockaddr *)&tcpd_addr, &tcpd_addr_len) < 0) error("receiving ACCEPT reply"); 
    if (buffer[0] != 'A') error("ACCEPT communication mismatch with TCPD");
	else printf("ACCEPT\n");
	return 1; 
}

// Nothing to do here
int BIND(int sockfd, struct sockaddr *my_addr, int addrlen)
{ return 0; }



/*Unimplemented methods*/
void LISTEN(int sockfd,int backlog)
{ return; }

int CONNECT(int sockfd, struct sockaddr *serv_addr, int addrlen)
{ return 0; }


