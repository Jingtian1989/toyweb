
#ifndef __RIO_H__
#define __RIO_H__

#define RIO_BUFSIZE 8192
typedef struct {
	int rio_fd;
	int rio_cnt; 				/* unread bytes in internal buffer */
	char *rio_bufptr; 			/* next unread byte in internal buffer */
	char rio_buf[RIO_BUFSIZE]; 	/* internal buffer*/
} rio_t;

void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
void rio_writen(int fd, void *usrbuf, size_t n);

#endif