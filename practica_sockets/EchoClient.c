/*
*TCP echo client based on the stream socket client demo 
*contained in Beej's Guide to Network Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define LINELEN         1500

int UDPecho(int fd);
void *get_in_addr(struct sockaddr *sa);


int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char* port;
	char* host;

	switch (argc) {
	case 3:
		host = argv[1];
		port = argv[2];
		break;
	default:
		fprintf(stderr, "usage: EchoClient host port\n");
		exit(1);
        }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		 
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

        //Just for printing
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

        UDPecho(sockfd);
      exit(0);
}

/*------------------------------------------------------------------------
 * TCPecho - send input to ECHO service on specified host and print reply
 *------------------------------------------------------------------------
 */
int UDPecho(int fd)
{
        char    buf[LINELEN+1];         /* buffer for one line of text  */
        int     n;                   	/* read count*/
        int     outchars, inchars;      /* characters sent and received */

    while (fgets(buf, LINELEN+1, stdin)) {
                buf[LINELEN] = '\0';    /* insure line null-terminated  */
                outchars = strlen(buf);
                send(fd, buf, outchars,0);

                /* read it back */
                for (inchars = 0; inchars < outchars; inchars+=n ) {
                        n = recv(fd, &buf[inchars], outchars - inchars, 0);
                        if (n < 0)
				fprintf(stderr, "socket read failed: %s\n",
                                		strerror(errno));
                }
                fputs(buf, stdout);
        }
        close(fd);

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

