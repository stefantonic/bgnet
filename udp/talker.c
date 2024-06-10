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

int
main(int argc, char *argv[]){

	struct addrinfo *parameters, *server_info, *ptr;
	int32_t get_address_info, sock_fd, number_of_bytes;

	parameters = malloc(sizeof(struct addrinfo));
	server_info = malloc(sizeof(struct addrinfo));
	ptr = malloc(sizeof(struct addrinfo));

	memset(parameters, 0, sizeof(*parameters));		
	memset(server_info, 0, sizeof(*server_info));
	memset(ptr, 0, sizeof(*ptr));

	parameters->ai_family = AF_UNSPEC;
	parameters->ai_socktype = SOCK_DGRAM;

	if (argc != 3){
		fprintf(stderr, "usage: talker hostname message\n");
	}	

	if ((get_address_info = getaddrinfo(argv[1], PORT, parameters, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_address_info));
		return 1;
	}

	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
		if ((sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1){
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (ptr == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
			
	if ((number_of_bytes = sendto(sock_fd, argv[2], strlen(argv[2]), 0,
					ptr->ai_addr, ptr->ai_addrlen)) == -1){
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(server_info);

	printf("talker: sent %d bytes to %s\n", number_of_bytes, argv[1]);
	close(sock_fd);

	return 0;
}
