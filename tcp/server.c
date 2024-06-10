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


#define PORT "3490"
#define BACKLOG 10	/* number of connections allowed on the incoming queue     */

void signal_child_handler(pid_t );	
void *get_in_addr(struct sockaddr *socket_address);


void signal_child_handler(pid_t process_id){	/*	SIGCHLD, sends to the shell
							                        whenever child job becomes 
                                                    zombie                     */

	u_int32_t saved_errno = errno;	
	pid_t p_process_id;
	
						/*	waitpid(-1),
							if the child process whose process
							group ID is equal to the absoluteof  pid	
							 
							WNOHANG,
							return immediately if
							no child exited			                           */
	while (1) {
		p_process_id = waitpid(WAIT_ANY, NULL, WNOHANG);
			if (p_process_id < 0) {
				perror("waitpid(): ");
				break;
			}
			if (p_process_id > 0) {
				break;
			}	
	}
	
	errno = saved_errno;	
}

void *get_in_addr(struct sockaddr *socket_address){
/*	IPv4 or IPv6	*/	
	if (socket_address->sa_family == AF_INET)
		return &(((struct sockaddr_in *)socket_address)->sin_addr);
	
/*	sockaddr_in:	
	sin_family 2b
	sin_port   2b
 	sin_addr   4b
 	sin_zero   8b 	 */

	return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);

/*	sockaddr_in6	
 	sin6_family   2b
 	sin6_port     2b
 	sin6_flowinfo 4b
 	sin6_addr     16b */
}


int 
main(int argc, char *argv[]){
/*	*parameters argument points to an addrinfo structure that
 	specifies criteria for selecting the socket address structures returned
 	in the list pointed to by *server_info		                               */
	
	const u_int8_t YES = 1;

	struct addrinfo *parameters, *server_info, *ptr;
	struct sockaddr_storage guest_address;
	struct sigaction __socket_address;	

	u_int16_t  sock_fd, new_sock_fd;
	socklen_t sin_size;		             /*	 32bit                             */
 
	char buffer[INET6_ADDRSTRLEN];	     /*  368bit                            */
	u_int32_t get_address_info;


	parameters = malloc(sizeof(struct addrinfo));
	server_info = malloc(sizeof(struct addrinfo));
       	ptr = malloc(sizeof(struct addrinfo));	
	memset(parameters, 0, sizeof(*parameters));
	memset(server_info, 0, sizeof(*server_info));	
        memset(ptr, 0, sizeof(*ptr));
	memset(&__socket_address, 0, sizeof(struct sigaction));	
/*	memset(signal_child_handler, 0, sizeof(signal_child_handler));	           */
	memset(&buffer, 0, sizeof(INET6_ADDRSTRLEN));	/*test*/

	printf("buffer size: %ld\taddress: %p\n", sizeof(buffer), &buffer);		
	
	parameters->ai_family = AF_UNSPEC;
	parameters->ai_socktype = SOCK_STREAM;
	parameters->ai_flags = AI_PASSIVE;

/*	if get_address_info is not NULL it points to an addrinfo structure whose
	ai_family, ai_socktype and ai_protocol specify criteria that limit the set of
	socket addresses returned by getaddrinfo()					               */ 

	if ((get_address_info = getaddrinfo(NULL, PORT, parameters, &server_info) != 0)) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_address_info));	
		return 1;
	}	

	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next){
		if ((sock_fd = socket(ptr->ai_family, ptr->ai_socktype, 
						ptr->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

/*	SOL_SOCKET, specifies the protocol level at the socket level
 	SO_REUSEADDR, specifies that the rules used in validating addresses
 	supplied to bind(), should allow reuse of local addresses
 	(if supported by the protocol)							                   */
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &YES,
					sizeof(u_int32_t)) == -1) {
			perror("server: setsockopt");
			continue;
		}

		if (bind(sock_fd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(sock_fd);
			perror("server: bind");
			continue;
		}

		break;
	}	
	
/*	freaddrinfo() function shall free one or more addrinfo structures returned
 	by getaddrinfo(), along with any additional storage associated with those structures
 	if the ai_next field of the structure is not null, the entire structures shall be freed
 											*/
	freeaddrinfo(server_info);	
	
	if (ptr == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
		
	if (listen(sock_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	


/*	system call used to change action taken by a process on receipt of a specific signal				
 	sa_handler specifies the action to be associated with signum			*/	
	__socket_address.sa_handler = signal_child_handler;
	sigemptyset(&__socket_address.sa_mask);	
	__socket_address.sa_flags = SA_RESTART; 
	
	if (sigaction(SIGCHLD, &__socket_address, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while (1) {
		sin_size = sizeof(guest_address);	
		new_sock_fd = accept(sock_fd, (struct sockaddr *)&guest_address, &sin_size);	
		if (new_sock_fd == -1) {
			perror("accept");
			continue;
		}
	
		inet_ntop(guest_address.ss_family,
				get_in_addr((struct sockaddr *)&guest_address), buffer, sizeof(buffer));
		printf("server: got connection from %s\n", buffer);
	

		if (!fork()) {
			close(sock_fd);
			if(send(new_sock_fd, "hello, chat", 12, 0) == -1)
				perror("server: failed to send message");
			close(new_sock_fd);
			exit(0);
		}

		close(new_sock_fd);
	}
	
/*	display information about sockaddr server_info					*/	
	while (server_info) {
		printf("ai_family: %d\n", server_info->ai_family);
		printf("ai_socktype: %d\n", server_info->ai_socktype);
		printf("ai_flags: %d\n", server_info->ai_flags);
		printf("ai_protocol: %d\n", server_info->ai_protocol);
	
		if (server_info->ai_family == AF_INET) {

			u_int32_t addr = htonl(((struct sockaddr_in *)server_info->ai_addr)->sin_addr.s_addr);
			printf("ai_addr: %u.%u.%u.%u\n",
				addr>>24, (addr>>16)&0xFF,
				(addr>>8)&0xFF, addr&0xFF);				
		}

		server_info = server_info->ai_next;
		putchar('\n');
	}

	return 0;
}
