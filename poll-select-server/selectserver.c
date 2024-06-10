#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define PORT "9034"
#define BUFFER (256)
#define BACKLOG (10)

void *get_in_addr(struct sockaddr *socket_address);
void *get_in_addr(struct sockaddr *socket_address){  
    if (socket_address->sa_family == AF_INET){
        return &(((struct sockaddr_in *)socket_address)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);
}

int
main(int argc, char *argv[]){

    struct addrinfo *parameters, *server_info, *ptr;
    struct sockaddr_storage remote_addr;
    socklen_t addr_len;

    fd_set master;
    fd_set read_fd;

    int32_t s_listener_fd, s_new_fd, s_fd_max, number_of_bytes;
    
    int32_t i, j, get_address_info;
    int32_t yes = 1;

    char _S_BUFFER1[BUFFER];
    char _S_BUFFER2[INET6_ADDRSTRLEN];    
    
/*  this macro clears (removes all file descriptors from) set.  
    It should be employed as the first step in initializing a 
    file descriptor set.                                                       */
    FD_ZERO(&master);
    FD_ZERO(&read_fd);
   

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

    for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
        s_listener_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s_listener_fd < 0){
            continue;
   
        }

/*      The setsockopt() API allows the application to reuse the local 
        address when the server is restarted before the required 
        wait time expires.                                                     */
        setsockopt(s_listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t));
       
        if (bind(s_listener_fd, ptr->ai_addr, ptr->ai_addrlen) < 0) {
            close(s_listener_fd);
            continue;
        }
        
        break; 
    }   

    if (ptr == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }    
    
    freeaddrinfo(server_info);

    if (listen(s_listener_fd, BACKLOG) == -1) {
        perror("listener");
        exit(3);
    } 


    FD_SET(s_listener_fd, &master);
    s_fd_max = s_listener_fd;

    for(;;){
/*      tryout FD_SETSIZE uint16_t(65535)                                                   
        for some class of I/O operation (e.g., input  possible).               */
        if (select(s_fd_max + 1, &read_fd, NULL, NULL, NULL) == -1){
            perror("select");
            exit(4);
        }
    
        for (i = 0; i <= s_fd_max; i++){
            if (FD_ISSET(i, &read_fd)){
                if (i == s_listener_fd) {
                    addr_len = sizeof(struct sockaddr_storage);
                    s_new_fd = accept(s_listener_fd, 
                            (struct sockaddr *)&remote_addr, &addr_len);  
                
                    if (s_new_fd == -1){
                       perror("accept");
                    } else {
                        FD_SET(s_new_fd, &master);
                        if (s_new_fd > s_fd_max) {
                            s_fd_max = s_new_fd;
                        }
                        printf("selectserver: new connection from %s on socket %d\n",
                                inet_ntop(remote_addr.ss_family, 
                                    get_in_addr((struct sockaddr *)&remote_addr),
                                    _S_BUFFER2, INET6_ADDRSTRLEN), s_new_fd);
                    }  
                } else {
                    if ((number_of_bytes = recv(i, _S_BUFFER1, sizeof(_S_BUFFER1), 0)) <= 0){
                        if (number_of_bytes == 0) {
                            printf("selectserver: socket %d hung up\n", i);    
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        for (j = 0; j <= s_fd_max; j++) {
                            if (FD_ISSET(j, &master)) {
                                if (j != s_listener_fd && j != i) {
                                    if (send(j, _S_BUFFER1, number_of_bytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
/*  (;;)                                                                       */
    }  
    return 0;
}
