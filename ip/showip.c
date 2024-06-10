#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>

int
main(int argc, char *argv[]){
/*	    struct addrinfo *parametes
		ai_flags
		ai_family
		ai_socktype
		...

		*tree_node->linked list, next node parameters included?	               */
	struct addrinfo *parameters, *tree_node, *ptr;
/*	struct addrinfo **parameters_result = &parameters;		                   */
	int status;
	char sockaddr_storage_[INET6_ADDRSTRLEN];	/*	(IPv6) socket address 
								                    variable-lenght-data	   */	
	
	if (argc != 2){
		fprintf(stderr, "usage: showip hostname\n");
		return 1;
	}
	
/*	sets n-bytes of dest to the value	                                       */
	memset(&sockaddr_storage_, 0, sizeof(sockaddr_storage_));
	parameters = malloc(sizeof(struct addrinfo));
	memset(parameters, 0, sizeof(*parameters));	
	parameters->ai_family = AF_UNSPEC;		/*	return IPv4 OR IPv6	           */
	parameters->ai_socktype = SOCK_STREAM;  /*	2B stream, 
								                socket type works like a pipe
								                man pipe	                   */	
	
	if ((status = getaddrinfo(argv[1], NULL, parameters, &tree_node)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}
	
	printf("IP addresses for %s:\n\n", argv[1]);
	
	for (ptr = tree_node; ptr != NULL; ptr = ptr->ai_next) {
		void *address_type;			        /*	IPv4 or IPv6	               */
		char *ip_type;				        /*	IPv4 or IPv6	               */
		
		if (ptr->ai_family == AF_INET) {
			struct sockaddr_in *IPv4 = (struct sockaddr_in *)ptr->ai_addr;
			address_type = &(IPv4->sin_addr);
			ip_type = "IPv4";
		} else {
			
			struct sockaddr_in6 *IPv6 = (struct sockaddr_in6 *)ptr->ai_addr;
			address_type = &(IPv6->sin6_addr);
			ip_type = "IPv6";
		}	

/*	    function shall convert a numeric address into a text string 	
	 	network-to-presentation -> network-to-printable			               */	
		inet_ntop(ptr->ai_family, address_type, sockaddr_storage_, 
				sizeof(sockaddr_storage_));
			
		printf("\t%s:\t%s\n", ip_type, sockaddr_storage_);
	}

	freeaddrinfo(tree_node);
	return 0;
}
