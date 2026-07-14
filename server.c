#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define CONFIRMATION_MSG "Server received your message successfully.\n"

static int create_listening_socket(void)
{
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

static void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    ssize_t bytes_sent;

    bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    if (bytes_received == 0) {
        fprintf(stderr, "Client disconnected before sending a message.\n");
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';

    printf("\nReceived:\n%s\n", buffer);

    printf("Sending confirmation...\n");

    bytes_sent = send(client_fd, CONFIRMATION_MSG, strlen(CONFIRMATION_MSG), 0);
    if (bytes_sent == -1) {
        perror("send");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    close(client_fd);
}

int main(void)
{
    int server_fd;
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    printf("Server started...\n");
    printf("Listening on port %d...\n", SERVER_PORT);

    server_fd = create_listening_socket();

    while (1) {
        client_addr_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("\nClient connected.\n");

        handle_client(client_fd);

        printf("\nWaiting for another client...\n");
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
