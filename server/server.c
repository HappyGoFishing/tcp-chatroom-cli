#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define FALLBACK_ADDRESS "127.0.0.1"
#define PORT 6675
#define BUFSIZE 1024
#define BACKLOG 5
#define MAX_CLIENTS 64

void err_n_die(char *msg) {
    perror(msg);
    exit(1);
}

void * handle_client(void *arg) {
    int *fd = (int *) arg;
    char buffer[BUFSIZE];

    recv(*fd, buffer, BUFSIZE, 0);
    printf("%s joined\n", buffer);
    while (true) {
        close(*fd);
        break;
    }
    return NULL;
}

int main(int argc, char **argv) {
    char address[64];
    if (argc < 2) {
        strcpy(address, FALLBACK_ADDRESS);
    } else {
        strcpy(address, argv[1]);
    }

    int server_fd, client_fd;

    int clients[MAX_CLIENTS]; // list of client file descriptors

    struct sockaddr_in server_addr, client_addr;
    socklen_t server_len = sizeof(server_addr);
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        err_n_die("socket");
    
    int sock_opt;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR| SO_REUSEPORT, &sock_opt, sizeof(sock_opt)) == -1)
        err_n_die("setsockopt");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        err_n_die("bind");

    if (listen(server_fd, BACKLOG) < 0)
        err_n_die("listen");
    
    printf("listening: %s\n", address);
    
    while (true) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        char client_ip_name[64];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_name, INET_ADDRSTRLEN);
        printf("established connection with: %s:%d\n", client_ip_name, ntohs(client_addr.sin_port));

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, &client_fd) != 0)
            err_n_die("pthread_create");
    }
    return 0;
}

