#include "functions.h"
#include <cerrno>
#include<iostream>

int printErrorAndExit(const char* errorMsg)
{
    puts(errorMsg);
    _exit(1);
}

ssize_t Send(int fd, __const void *buf, size_t n, struct sockaddr_in* sendAddr)
{
    //Send data to tcpd port
    ssize_t retVal = sendto(fd, buf, n, 0,
            (struct sockaddr *)sendAddr,
            sizeof(*sendAddr));
    if (retVal < 0) {
	    std::cout << strerror(errno) << std::endl;
        printErrorAndExit("Error sending data.\n");
	}	
		
    usleep(10000);
    return retVal;
}

ssize_t Recv(int fd, void* buf, size_t n)
{
    return recvfrom(fd, buf, n, 0, NULL, NULL);
}

ssize_t Read(int fd, void *buf, size_t nbytes)
{
    ssize_t size;
    size = read(fd, buf, nbytes);
    if (size < 0)
        printErrorAndExit("Error reading data from file.\n");
    return size;
}
