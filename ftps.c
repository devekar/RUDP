#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


#include "user_library.h"


#define CHUNK_SIZE 1000
#define FILESIZE_SIZE 4
#define FILENAME_SIZE 20
#define SERVER_DIR_PATH "server_dir/"



int receive_filesize(int newsockfd)
{
	int tmp, recv_bytes, filesize;
    if( (recv_bytes = RECV(newsockfd, &tmp, FILESIZE_SIZE, 0)) != FILESIZE_SIZE) error("File size not received correctly");
    filesize = ntohl(tmp);
    printf("File size: %d bytes\n", filesize);
	return filesize;
}


void receive_filename(char file_path[], int newsockfd)
{
	int recv_bytes;
	char filename[FILENAME_SIZE];
	if( (recv_bytes = RECV(newsockfd, filename, FILENAME_SIZE, 0)) != FILENAME_SIZE) error("File name not received correctly");
	strcpy(file_path, SERVER_DIR_PATH);
	strcat(file_path, filename);
	printf("File will be written to: %s\n", file_path);
}


void receive_file(char file_path[], int filesize, int newsockfd)
{
	int recv_bytes, write_bytes, total_bytes;
	char file_buf[CHUNK_SIZE];
	
	FILE *f = fopen(file_path,"wb");
	if(!f) error("Cannot open file");

	printf("Will now receive file\n");

	total_bytes = 0;
	while( total_bytes < filesize) {
	  if ( (recv_bytes = RECV(newsockfd,file_buf,CHUNK_SIZE, 0)) > 0) {
			fwrite(file_buf, sizeof(char), recv_bytes, f);
			total_bytes += recv_bytes;
			printf("\rTotal bytes: %d", total_bytes);
			fflush(stdout);
	  }
	  else printf("\nError: Did not receive data. Retrying...\n");
	}
	
	printf("\nReceive Complete: Total bytes = %d\n", total_bytes);
	fclose(f);
}




int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	char file_path[255];
	int filesize;

	if (argc < 2) {
	 fprintf(stderr,"ERROR, no port provided\n");
	 exit(1);
	}

	// Get socket descriptor
	sockfd = SOCKET(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	// Fill sockaddr_in structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// Bind socket to port
	if (BIND(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		  error("ERROR on binding");

	// Notify kernel for incoming connections
	LISTEN(sockfd,5);

	// Accept a connection and get new socket descriptor
	clilen = sizeof(cli_addr);
	newsockfd = ACCEPT(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) error("ERROR on accept");

	filesize = receive_filesize(newsockfd);
	receive_filename(file_path, newsockfd);
	receive_file(file_path, filesize, newsockfd);

    CLOSE(newsockfd);
    CLOSE(sockfd);
    return 0; 
}
