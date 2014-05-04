#include "toyweb.h"


static int tw_open_listen1(int port)
{
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
		return -1;

	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
		return -1;

	if (listen(listenfd, LISTENQ) < 0)
		return -1;
	return listenfd;
}

int tw_open_listen(int port)
{
	int rc;

	if ((rc = tw_open_listen1(port)) < 0)
		unix_error("tw_open_listen failed");
	return rc;
}

int tw_accept(int socket, struct sockaddr *addr, int *addrlen)
{
	int rc;
	if ((rc = accept(socket, addr, addrlen)) < 0)
		unix_error("tw_accept failed");
	return rc;
}


int tw_open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
		unix_error("tw_open failed");
    return rc;
}

void *tw_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
		unix_error("tw_mmap failed");
    return(ptr);
}

void tw_munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
		unix_error("tw_munmap failed");
}

void tw_close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
		unix_error("tw_close failed");
}

int tw_dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
		unix_error("tw_dup2 failed");
    return rc;
}

pid_t tw_fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
		unix_error("tw_fork failed");
    return pid;
}

void tw_execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
		unix_error("tw_execve failed");
}

pid_t tw_wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
		unix_error("tw_wait failed");
    return pid;
}

static void tw_handle_request_error(int fd, char *cause, char *errornum, char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	// Build HTTP response body
	sprintf(body, "<html><title>ToyWeb Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errornum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The ToyWeb Server</em>\r\n", body);

	// Print the HTTP response
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errornum, shortmsg);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	rio_writen(fd, buf, strlen(buf));
	rio_writen(fd, body, strlen(body));
}

static void tw_read_request_headers(rio_t *rp)
{
	char buf[MAXLINE];
	rio_readlineb(rp, buf, MAXLINE);
	while(strcmp(buf, "\r\n"))
	{
		rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}


static int  tw_parse_request_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
	if (!strstr(uri, "cgi-bin"))
	{
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri)-1] == '/')
			strcat(filename, "home.html");
		return 1;
	}else 
	{
		ptr = index(uri, '?');
		if (ptr) 
		{
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}else
		{
			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}


}

static void tw_get_request_file_type(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
    else
		strcpy(filetype, "text/plain");
}

static void tw_serve_request_static(int fd, char *filename, int filesize)
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	// Send response headers
	tw_get_request_file_type(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: ToyWeb\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	rio_writen(fd, buf, strlen(buf));

	// Send response body
	srcfd = tw_open(filename, O_RDONLY, 0);
	srcp = tw_mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	tw_close(srcfd);
	rio_writen(fd, srcp, filesize);
	tw_munmap(srcp, filesize);

}

static void tw_serve_request_dynamic(int fd, char *filename, char *cgiargs)
{
	char buf[MAXLINE], *emptylist[] = {NULL};

	// Return first part of HTTP response
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: ToyWeb\r\n");
	rio_writen(fd, buf, strlen(buf));

	if (tw_fork() == 0)
	{
		setenv("QUERY_STRING", cgiargs, 1);
		tw_dup2(fd, STDOUT_FILENO);
		tw_execve(filename, emptylist, environ);
	}
	tw_wait(NULL);
}

static void tw_handle_request(int fd)
{
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	rio_readinitb(&rio, fd);
	rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET"))
	{
		tw_handle_request_error(fd, method, "501", "Not Implemented", 
			"ToyWeb does not implemented this method");
		return;
	}
	tw_read_request_headers(&rio);

	is_static = tw_parse_request_uri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0)
	{
		tw_handle_request_error(fd, filename, "404", "Not Found", 
			"ToyWeb couldn't find this file");
		return;
	}

	if (is_static)
	{
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
		{
			tw_handle_request_error(fd, filename, "403", "Forbidden", 
				"ToyWeb couldn't read the file");
			return;
		}
		tw_serve_request_static(fd, filename, sbuf.st_size);
	}else 
	{
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
		{
			tw_handle_request_error(fd, filename, "403", "Forbidden",
				"ToyWeb couldn't run the CGI program");
			return;
		}
		tw_serve_request_dynamic(fd, filename, cgiargs);
	}
}


int main(int argc, char const *argv[])
{
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	listenfd = tw_open_listen(port);
	while (1)
	{
		clientlen = sizeof(clientaddr);
		connfd = tw_accept(listenfd, (SA *)&clientaddr, &clientlen);
		tw_handle_request(connfd);
		tw_close(connfd);
	}
	return 0;
}



