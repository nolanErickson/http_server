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

#define LOCALPORT "6969"
#define BACKLOG 5

// getaddrinfo();
// socket();
// bind();
// listen();
/* accept() goes here */


int reuse_addr = 1;
struct sigaction sa;
char client_ip[INET6_ADDRSTRLEN];

int status;
int server_fd;
int client_fd;
socklen_t client_size;
struct sockaddr_storage client_addr;
struct addrinfo hints, *servinfo, *p;

void sigchld_handler(int s) {

	(void)s;
	
	//waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0) {

		errno = saved_errno;
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main() {

//server.h
	//store hints, then store all address info
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	status = getaddrinfo("localhost", LOCALPORT, &hints, &servinfo);
	if (status != 0) {
		fprintf(stderr, "gai error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	//loop through all possible server addresses until one binds
	for (p = servinfo; p != NULL; p = p->ai_next) {
	// Makes the socket
		server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (server_fd == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
		//binds it
        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("server: bind");
            continue;
        }
		break;
	}
	//free servinfo
	freeaddrinfo(servinfo);

	if(p == NULL) {
		printf("server: failed to bind\n");
		exit(1);
	}
	//starts listening
	if (listen(server_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections...\n");
//server.h

	//register child process handler to prevent zombie processes
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	while(1) {
		client_size = sizeof(client_addr);
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
		if (client_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip));
		printf("server: got connection from %s\n", client_ip);
		if (!fork()) {
			close(server_fd);
			if (send(client_fd, "Hello world!", 13, 0) == -1);
				perror("send");
			close(client_fd);
			exit(0);
		}
		close(client_fd);

	}
	return 0;
		

	
	// //implement this into logic above
	// // Accept incoming connections
	// client_size = sizeof(client_addr);
	// client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);

	// //send confirmation of connection
	// char *msg = "HTTP/1.1 200 OK\r\n\n";
	// int len, bytes_sent;

	// len = strlen(msg);
	// bytes_sent = send(client_fd, msg, len, 0);


	// close(server_fd);
	// freeaddrinfo(servinfo);

	// return 0;
}


