#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "constants.h"



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
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) error("binding");

	return local_addr;
}

// create sockaddr_in for target end
struct sockaddr_in make_remote_addr(char host[], int port)
{
	struct hostent *hp, *gethostbyname();
	struct sockaddr_in remote_addr;
	
	/* construct name for connecting to server */
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(port);

	/* convert hostname to IP address and enter into name */
	hp = gethostbyname(host);
	if(hp == 0) error("unknown host\n");
	bcopy((char *)hp->h_addr, (char *)&remote_addr.sin_addr, hp->h_length);
	
	return remote_addr;
}


int main(int argc, char *argv[])
{
    int in_sockfd, out_sockfd, recv_sockfd;
	int in_addr_len, out_addr_len, recv_addr_len;
    struct sockaddr_in in_addr, out_addr, recv_addr;
	char buffer[MSS + 1];
	int recv_bytes, sent_bytes, buflen;
	
    /*create socket for IPC with user process and bind it*/
    in_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(in_sockfd < 0) error("opening datagram socket");
	in_addr = make_addr_and_bind(in_sockfd, TCPD_IN_PORT);
	in_addr_len = sizeof(in_addr);
	
	
	/*create socket for troll communication and bind it*/
	out_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(out_sockfd < 0) error("opening datagram socket");
	out_addr = make_addr_and_bind(out_sockfd, TCPD_OUT_PORT);
	
	//reuse out_addr to define target end i.e. troll
	out_addr = make_remote_addr(LOCALHOST, TROLL_IN_PORT);
	out_addr_len = sizeof(out_addr);
	
	
	/*create socket for receiving communication and bind it*/
	recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(recv_sockfd < 0) error("opening datagram socket");
	recv_addr = make_addr_and_bind(recv_sockfd, TCPD_RECV_PORT);
	recv_addr_len = sizeof(recv_addr);	
	
	
	while(1) {
		// wait for message from local user process
		recv_bytes = recvfrom(in_sockfd, buffer, MSS + 1, 0, (struct sockaddr *)&in_addr, &in_addr_len);
		if( recv_bytes < 0) error("error receiving from user");

		// Take appropriate action based on control byte
		switch(buffer[0])
		{
			case 'X' :  // Test communication by sending back same data
				sent_bytes = sendto(in_sockfd, buffer, recv_bytes, 0, (struct sockaddr *)&in_addr, in_addr_len);
				if(sent_bytes < recv_bytes) error("sending X back");
				printf("SOCKET call\n");
				break;
				
			case 'S':   //Send data to troll excluding control byte
				sent_bytes = sendto(out_sockfd, buffer + 1, recv_bytes - 1, 0, (struct sockaddr *)&out_addr, out_addr_len);
				if(sent_bytes < recv_bytes - 1) error("sending packet to troll");
				printf("TCPD sends %d bytes\n", recv_bytes - 1);
				break;
				
			case 'R':  // User says we will receive data from remote host
				recv_bytes = recvfrom(recv_sockfd, buffer, MSS, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
				if(recv_bytes < 0) error("receiving packet from remote host");
				
				// Send data to local user process
				sent_bytes = sendto(in_sockfd, buffer, recv_bytes, 0, (struct sockaddr *)&in_addr, in_addr_len);
				if(sent_bytes < recv_bytes) error("sending received message to user");
				printf("TCPD receives %d bytes\n", recv_bytes);
				break;
				
			case 'B':
			case 'L':
			case 'A':
			case 'C':
			case 'Y':
				break;
				
			default:  //Unknown control byte
				printf("Invalid message: %c  %.*s\n", buffer[0], recv_bytes -1 , buffer + 1);
				break;
		}
	}
	
    close(in_sockfd);	
	close(out_sockfd);
	close(recv_sockfd);
	return 0;
}
