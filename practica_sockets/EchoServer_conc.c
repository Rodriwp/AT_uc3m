/*
* TCP echo concurrent server based on the stream socket server demo contained in
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
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold

#define BUFSIZE         4096


void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
int UDPechod(int fd);

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
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
		perror("usage: EchoServer_conc [port]\n");
        }


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM; //select UPD socket type
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

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
                } // Set an option for the socket in this case S0_REUSEADDR

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

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
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

                // just for printing
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
                //printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
	
                        UDPargechod(new_fd);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

/*------------------------------------------------------------------------
 * sigchld_handler - reaper of zombi child processes
 *------------------------------------------------------------------------
 */
void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
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

/*------------------------------------------------------------------------
 * UDPechod - echo data until end of file
 *------------------------------------------------------------------------
 */
int UDPechod(int fd)
{
	char	buf[BUFSIZ];
	int	cc;
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

	return 0;
}




