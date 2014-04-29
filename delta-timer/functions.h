#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/mman.h>

extern int printErrorAndExit(char* errorMsg);

extern ssize_t Send(int fd, __const void *buf, size_t n, struct sockaddr_in* sendAddr);

extern ssize_t Recv(int fd, void* buf, size_t n);

extern ssize_t Read(int fd, void *buf, size_t nbytes);

extern void *Mmap (void *__addr, size_t __len, int __prot,
        int __flags, int __fd, __off_t __offset);


#endif //!_FUNCTIONS_H_
