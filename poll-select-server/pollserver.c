#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9034"
#define BUFFER (256)
#define BACKLOG (10)

void *get_in_addr(struct sockaddr *);
int get_listener_socket(void);
void add_to_pollfd(struct pollfd *fdarray[], int32_t nfds, 
		int32_t *nfds_count, int32_t *nfds_size);
void del_from_pollfd(struct pollfd fdarray[], int32_t nfds, int32_t *nfds_size); 

void *get_in_addr(struct sockaddr *socket_address){

	if (socket_address->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)socket_address)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);
}

int get_listener_socket(void){
	
	struct addrinfo *parameters, *server_info, *ptr;

	int32_t s_listener_fd, get_address_info;
	int32_t yes = 1;

	parameters = malloc(sizeof(struct addrinfo));
	server_info = malloc(sizeof(struct addrinfo));
	ptr = malloc(sizeof(struct addrinfo));

	memset(parameters, 0, sizeof(*parameters));
	memset(server_info, 0, sizeof(*server_info));
	memset(ptr, 0, sizeof(*ptr));

	parameters->ai_family = AF_UNSPEC;
	parameters->ai_socktype = SOCK_STREAM;
	parameters->ai_flags = AI_PASSIVE;

	if ((get_address_info = getaddrinfo(NULL, PORT, parameters, &server_info)) != 0){
		fprintf(stderr, "selectserver: %s\n", gai_strerror(get_address_info));
		exit(1);	
	}	

	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next){
		s_listener_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (s_listener_fd < 0) {
			continue;
		}
		
		setsockopt(s_listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t));
		
		if (bind(s_listener_fd, ptr->ai_addr, ptr->ai_addrlen) < 0){
			close(s_listener_fd);
			continue;
		}

		break;
	}

	freeaddrinfo(server_info);
	if (ptr == NULL) {
		return -1;
	}

/*	backlog argument provides a hint to the implementation which the
    implementation shall use to limit the number of outstanding connections 
    in the socket's listen queue	                                           */
	if (listen(s_listener_fd, BACKLOG) == -1){
		return -1;
	}

	return s_listener_fd;
}

void add_to_pollfd(struct pollfd *fdarray[], int32_t nfds,
		int32_t *nfds_count, int32_t *nfds_size){

/*	adding new file descriptor to the set
 	if the number of elements within array is equal to max val, max val * 2	   */	
	if (*nfds_count == *nfds_size){
		*nfds_size *= 2;	
/*		realloc->expand the existing area pointed by *nfds_size				   */
		*fdarray = realloc(*fdarray, sizeof(**fdarray) * (*nfds_size));		
	}

	(*fdarray)[*nfds_count].fd = nfds;
	
/*	events->telling kernel which events we are interested in for each fd
	POLLIN->data other than high priority data can be read 					   */
	(*fdarray)[*nfds_count].events = POLLIN;
	
	(*nfds_count)++;
}	

void del_from_pollfd(struct pollfd fdarray[], int32_t nfds, int32_t *nfds_count){
	
	fdarray[nfds] = fdarray[*nfds_count - 1];	
	(*nfds_count)--;

}

int
main(int argc, char *argv[]){


	struct sockaddr_storage remote_addr;		
	int32_t s_listener_fd, s_new_fd;
/*	int32_t socklen_t									                       */ 
	socklen_t addr_len;

	char _SS_BUFFER1[BUFFER];
	char _SS_BUFFER2[INET6_ADDRSTRLEN];

	memset(&_SS_BUFFER1, 0, sizeof(BUFFER));
	memset(&_SS_BUFFER2, 0, sizeof(INET6_ADDRSTRLEN));

	int32_t nfds_count = 0;
	int32_t nfds_size  = 5;
	struct pollfd *pollfd = malloc(sizeof *pollfd * nfds_size);	
	
	s_listener_fd = get_listener_socket();	
	
	if (s_listener_fd == -1){
		fprintf(stderr, "error: getting listening socket\n");
		exit(1);
	}

	pollfd[0].fd = s_listener_fd;
	pollfd[0].events = POLLIN;

	nfds_count = 1;

	for (;;) {
		int32_t poll_count = poll(pollfd, nfds_count, -1);
		
		if (poll_count == -1){
			perror("poll");
			exit(1);
		}
	
		for (int32_t i = 0; i < nfds_count; i++) {
/*          has there been any occuring event                                  */
            if (pollfd[i].revents & POLLIN){
				
                if (pollfd[i].fd == s_listener_fd) {
					addr_len = sizeof(remote_addr);
					s_new_fd = accept(s_listener_fd, (struct sockaddr *)&remote_addr,
                            &addr_len);
					
					if (s_new_fd == -1) {
						perror("accept");
					} else {
						add_to_pollfd(&pollfd, s_new_fd, &nfds_count, &nfds_size);	
						printf("pollserver: new connection from %s on  socket %d\n",
							inet_ntop(remote_addr.ss_family, 
								get_in_addr((struct sockaddr *)&remote_addr),	
								_SS_BUFFER2, INET6_ADDRSTRLEN), s_new_fd);
					}
			} else {
				int32_t number_of_bytes = recv(pollfd[i].fd, _SS_BUFFER1, 
                        sizeof(_SS_BUFFER1), 0);
				int32_t s_sender_fd = pollfd[i].fd; 

				if (number_of_bytes <= 0){
					if (number_of_bytes == 0){
							printf("pollserver: socket %d hung up\n", s_sender_fd);
					} else {
							perror("recv");
					}
					
					close(pollfd[i].fd);
					del_from_pollfd(pollfd, i, &nfds_count);
			} else {

					for (int32_t j = 0; j < nfds_count; j++) {
							int32_t s_dest_fd = pollfd[j].fd;
							if (s_dest_fd != s_listener_fd && s_dest_fd != s_sender_fd) {
								if (send(s_dest_fd, _SS_BUFFER1, number_of_bytes, 0) == -1){
                                    perror("send");		
								}
							}
						}
					}
				}	
			}
		}
	}

	return 0;
}
