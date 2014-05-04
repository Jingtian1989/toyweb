#ifndef __TOY_WEB_H__
#define __TOY_WEB_H__

#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "rio.h"

typedef struct sockaddr SA;

#define LISTENQ 1024	/* second argument to listen() */
#define MAXLINE 8192	/* max text line length */
#define MAXBUF 	8192	/* max I/O buffer size */

extern char **environ;

int 	tw_open_listen(int port);
int 	tw_accept(int socket, struct sockaddr *addr, int *addrlen);
int  	tw_open(const char *pathname, int flags, mode_t mode);
void*	tw_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void 	tw_munmap(void *start, size_t length); 
void 	tw_close(int fd);
int  	tw_dup2(int fd1, int fd2);
pid_t 	tw_fork(void);
void 	tw_execve(const char *filename, char *const argv[], char *const envp[]);
pid_t 	tw_wait(int *status); 

#endif
