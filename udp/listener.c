#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "4950"
#define MAX_DATA_SIZE 100

void *get_in_addr(struct sockaddr *);
void *get_in_addr(struct sockaddr *socket_address){
	
	if (socket_address->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)socket_address)->sin_addr);	
	}

	return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);
}

int
main(int argc, char *argv[]){

	
	struct addrinfo *parameters, *server_info, *ptr;
	struct sockaddr_storage *guest_addr;
	socklen_t addr_len;

	int32_t get_address_info, sock_fd, number_of_bytes;
       
	char _ss_buffer1[MAX_DATA_SIZE];
	char _ss_buffer2[INET6_ADDRSTRLEN];
		
	parameters = malloc(sizeof(struct addrinfo));
	server_info = malloc(sizeof(struct addrinfo));
	ptr = malloc(sizeof(struct addrinfo));
	guest_addr = malloc(sizeof(struct sockaddr_storage)); 

	memset(parameters, 0, sizeof(*parameters));
	memset(server_info, 0, sizeof(*server_info));	
	memset(ptr, 0, sizeof(*ptr));	
	memset(guest_addr, 0, sizeof(*guest_addr));
	
	memset(&_ss_buffer1, 0, sizeof(MAX_DATA_SIZE));
	memset(&_ss_buffer2, 0, sizeof(INET6_ADDRSTRLEN));

	parameters->ai_family = AF_UNSPEC;
	parameters->ai_socktype = SOCK_DGRAM;
	parameters->ai_flags = AI_PASSIVE; 

	
	if ((get_address_info = getaddrinfo(NULL, PORT, parameters, &server_info)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_address_info));
		return 1;
	}

	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
		if ((sock_fd = socket(ptr->ai_family, ptr->ai_socktype,
						ptr->ai_protocol)) == -1){
			perror("listener: socket");
			continue;
		}
		
		if (bind(sock_fd, ptr->ai_addr, ptr->ai_addrlen) == -1){
			close(sock_fd);
			perror("listener: bind");
			continue;

		}
		break;
	}
	
	if (ptr == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(server_info);
	printf("listener: waiting to recvfrom...\n");

 	addr_len = sizeof(struct sockaddr_storage);	
	if ((number_of_bytes = recvfrom(sock_fd, _ss_buffer1, MAX_DATA_SIZE - 1, 0, 
					(struct sockaddr *)guest_addr, &addr_len)) == -1){
		perror("recvfrom()");
		exit(1);
	}

	printf("listener: got packet from %s\n", 
			inet_ntop(guest_addr->ss_family,
			get_in_addr((struct sockaddr *)&guest_addr),
			_ss_buffer2, sizeof(_ss_buffer2)));
	printf("listener: packet is %d bytes long\n", number_of_bytes);
	_ss_buffer1[number_of_bytes] = '\0';
	printf("listener: packet contains \"%s\"\n", _ss_buffer1);
		
	free(parameters);

	return 0;
}
