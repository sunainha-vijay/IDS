#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
} ClientData;

void *handle_client(void *arg) {
    ClientData *client = (ClientData *)arg;
    char buffer[BUFFER_SIZE];

    printf("[+] New client connected: %s:%d\n",
           inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            printf("[-] Client disconnected: %s:%d\n",
                   inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            close(client->socket);
            free(client);
            pthread_exit(NULL);
        }

        printf("[Client %d]: %s", client->socket, buffer);

        // Echo the message back
        send(client->socket, buffer, strlen(buffer), 0);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[-] Error creating socket");
        return EXIT_FAILURE;
    }

    // Configure server settings
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Error binding socket");
        return EXIT_FAILURE;
    }

    // Start listening for clients
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[-] Error listening");
        return EXIT_FAILURE;
    }

    printf("[+] Server started on port %d...\n", PORT);

    while (1) {
        // Accept client connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("[-] Error accepting client");
            continue;
        }

        // Allocate memory for client data
        ClientData *client = (ClientData *)malloc(sizeof(ClientData));
        client->socket = client_fd;
        client->address = client_addr;

        // Create a new thread for the client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client) != 0) {
            perror("[-] Error creating thread");
            free(client);
            close(client_fd);
        }
        pthread_detach(thread_id);  // Automatically free thread resources
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
