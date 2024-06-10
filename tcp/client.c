#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"
#define MAX_DATA_SIZE 255

void *get_in_addr(struct sockaddr *socket_address);
void *get_in_addr(struct sockaddr *socket_addres){
	
	if (socket_addres->sa_family == AF_INET)
		return &(((struct sockaddr_in *)socket_addres)->sin_addr);
	
	return &(((struct sockaddr_in6 *)socket_addres)->sin6_addr);
}

/*	telnet remotehostname 3490						                           */
int 
main(int argc, char *argv[]){

	
	struct addrinfo *parameters, *server_info, *ptr;
	uint32_t get_address_info, sock_fd, number_of_bytes;
	char _ss_buffer1[MAX_DATA_SIZE];
	char _ss_buffer2[INET6_ADDRSTRLEN];
		
	if (argc != 2){
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	parameters = malloc(sizeof(struct addrinfo));
	server_info = malloc(sizeof(struct addrinfo));
	ptr = malloc(sizeof(struct addrinfo));

	memset(parameters, 0, sizeof(*parameters));
	memset(server_info, 0, sizeof(*server_info));
	memset(ptr, 0, sizeof(*ptr));
	memset(&_ss_buffer1, 0, sizeof(MAX_DATA_SIZE));
	memset(&_ss_buffer2, 0, sizeof(INET6_ADDRSTRLEN));

	if ((get_address_info = getaddrinfo(argv[1], PORT, parameters, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_address_info));
		return 1;
	}
	
	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
		if ((sock_fd = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

	
		if (connect(sock_fd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(sock_fd);
			perror("client: connect");
			continue;
		}
	
		break;
	}	

	if (ptr == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}


/*	ntop() converts the network address structure src in the AF into a character 
	string resulting string is copied to the buffer pointed to by DST, which must be
 	a non-null pointer 								                           */
	inet_ntop(ptr->ai_family, get_in_addr((struct sockaddr *)ptr->ai_addr),
			_ss_buffer2, sizeof(_ss_buffer2));
	printf("client: connecting to %s\n", _ss_buffer2);
	
	freeaddrinfo(server_info);
	

	if ((number_of_bytes = recv(sock_fd, _ss_buffer1, MAX_DATA_SIZE - 1, 0)) == -1){
		perror("recv");
		exit(1);
	}	

/*	rr										*/
	_ss_buffer1[number_of_bytes] = '\0';
	printf("client: received '%s'\n", _ss_buffer1);
	
	close(sock_fd);	


	return 0;
}
