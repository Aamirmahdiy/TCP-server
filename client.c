#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 1024
#define EXIT_CMD    "exit"

static void strip_newline(char *buf)
{
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
}

static int is_exit_message(const char *msg)
{
    return strcmp(msg, EXIT_CMD) == 0;
}

static ssize_t recv_message(int fd, char *buf, size_t size)
{
    ssize_t bytes_received;

    bytes_received = recv(fd, buf, size - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        return -1;
    }
    if (bytes_received == 0) {
        return 0;
    }

    buf[bytes_received] = '\0';
    return bytes_received;
}

static void send_message(int fd, const char *msg)
{
    if (send(fd, msg, strlen(msg), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

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

    strip_newline(buf);
}

int main(void)
{
    int client_fd;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t bytes_received;

    client_fd = create_connected_socket();

    /* Communication loop: send message, wait for server reply, repeat until exit. */
    while (1) {
        read_user_message(message, sizeof(message));
        send_message(client_fd, message);

        if (is_exit_message(message)) {
            break;
        }

        bytes_received = recv_message(client_fd, response, sizeof(response));
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                fprintf(stderr, "Server closed the connection.\n");
            }
            break;
        }

        printf("\nServer:\n%s\n", response);

        if (is_exit_message(response)) {
            break;
        }
    }

    close(client_fd);
    printf("\nConnection closed.\n");
    return EXIT_SUCCESS;
}
