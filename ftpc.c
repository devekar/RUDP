#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 



#include "reliable_udp.h"


#define CHUNK_SIZE 512
#define FILESIZE_SIZE 4
#define FILENAME_SIZE 20



struct sockaddr_in make_sock_addr(char host[], int portno)
{
    struct hostent *server;
	struct sockaddr_in serv_addr;
	
    // Get server details from hostname
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // Fill the sockaddr_in structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
 
    return serv_addr;
}


void send_filesize(char filename[], int sockfd)
{
   	FILE *f = fopen(filename,"rb");
    if(!f) error("Cannot open file");
    fseek(f, 0L, SEEK_END);
    int file_size = ftell(f);
    fclose(f);

    printf("File size: %d bytes\n", file_size);
    int tmp = htonl((uint32_t)file_size);

    // Send file size and name
    int sent_bytes = SEND(sockfd, &tmp, FILESIZE_SIZE, 0);
    if(sent_bytes < FILESIZE_SIZE) error("ERROR writing to socket"); 
}


void send_filename(char filename[], int sockfd)
{
	char buffer[FILENAME_SIZE];
    if(strlen(filename) > FILENAME_SIZE -1 ) error("File name is larger than 19 characters.\n");
	strcpy(buffer, filename);
    int sent_bytes = SEND(sockfd, buffer, FILENAME_SIZE, 0);
    if(sent_bytes < FILENAME_SIZE) error("ERROR writing to socket"); 
}


void send_file(char filename[], int sockfd)
{
	int read_bytes, total_bytes, sent_bytes;
	char file_buf[CHUNK_SIZE];
	
   	FILE *f = fopen(filename,"rb");
    if(!f) error("Cannot open file");
	
	printf("Will now send file.\n");
	total_bytes = 0;
    // TODO: retry send when less bytes sent or not sent
    while ( (read_bytes = fread(file_buf, sizeof(char), CHUNK_SIZE, f)) > 0 ) {
            sent_bytes = SEND(sockfd, file_buf, read_bytes, 0);
            if(sent_bytes < read_bytes) error("ERROR writing to socket"); 
            total_bytes += sent_bytes;
			printf("Total bytes: %d\n", total_bytes);
    }
	fclose(f);
	
    printf("\nSend Complete: Total bytes = %d\n", total_bytes);
}


int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
	int n;
	
    if (argc < 4) {
       fprintf(stderr,"Usage: hostname port file\n");
       exit(0);
    }

    // Get socket descripter
    sockfd = SOCKET(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    serv_addr = make_sock_addr(argv[1], atoi(argv[2]));

    // Connect to server
    if ( CONNECT(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0 ) 
        error("ERROR connecting");

	
	send_filesize(argv[3],sockfd);
	send_filename(argv[3],sockfd);
    send_file(argv[3], sockfd);
    
    CLOSE(sockfd);
    return 0;
}
