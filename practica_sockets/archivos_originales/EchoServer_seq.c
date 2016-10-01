/*
* TCP echo server based on the stream socket server demo contained in
* Beej's Guide to Network Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 10	 // how many pending connections queue will  hold
#define BUFSIZE         4096


void    TCPechod(int fd);
void   *get_in_addr(struct sockaddr *sa);


int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char* port;

	switch (argc) {
      	case    1:
		break;
	case    2:
		port = argv[1];
		break;
	default:
        	perror("usage: EchoServer_seq port\n");
        }


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		int yes=1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	//printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		//printf("server: got connection from %s\n", s);

		TCPechod(new_fd);
                close(new_fd);

	}
		

	return 0;
}

/*------------------------------------------------------------------------
 * TCPechod - echo data until end of file
 *------------------------------------------------------------------------
 */
void
TCPechod(int fd)
{
        char    buf[BUFSIZ];
        int     cc;
        while (cc = recv(fd, buf, sizeof(buf),0)) {
                if (cc < 0){
                        fprintf(stderr, "echo read: %s\n", strerror(errno));
                        exit(1);
			}

                write(0,buf,cc); /* stdin is the 0 file descriptor */

                if (send(fd, buf, cc, 0) < 0){
                        fprintf(stderr, "echo write: %s\n", strerror(errno));
                        exit(1);
			}
        }
        return;
}

/*------------------------------------------------------------------------
 * get_in_addr - get sockaddr, IPv4 or IPv6
 *------------------------------------------------------------------------
 */
void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

