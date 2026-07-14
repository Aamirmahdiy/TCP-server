#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 1024

static int create_connected_socket(void)
{
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid address: %s\n", SERVER_IP);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    return client_fd;
}

static void read_user_message(char *buf, size_t size)
{
    printf("Enter a message:\n");

    if (fgets(buf, (int)size, stdin) == NULL) {
        fprintf(stderr, "Failed to read input.\n");
        exit(EXIT_FAILURE);
    }

    /* Strip trailing newline before sending. */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
}

int main(void)
{
    int client_fd;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t bytes_sent;
    ssize_t bytes_received;

    client_fd = create_connected_socket();

    read_user_message(message, sizeof(message));

    bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent == -1) {
        perror("send");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    bytes_received = recv(client_fd, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    if (bytes_received == 0) {
        fprintf(stderr, "Server closed the connection unexpectedly.\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    response[bytes_received] = '\0';

    printf("\nServer replied:\n%s", response);

    close(client_fd);
    return EXIT_SUCCESS;
}
