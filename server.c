#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
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

static void read_line(const char *prompt, char *buf, size_t size)
{
    printf("%s", prompt);

    if (fgets(buf, (int)size, stdin) == NULL) {
        fprintf(stderr, "Failed to read input.\n");
        exit(EXIT_FAILURE);
    }

    strip_newline(buf);
}

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
    char client_msg[BUFFER_SIZE];
    char reply[BUFFER_SIZE];
    ssize_t bytes_received;

    /* Communication loop: receive client message, prompt server user for reply. */
    while (1) {
        bytes_received = recv_message(client_fd, client_msg, sizeof(client_msg));
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                fprintf(stderr, "Client disconnected.\n");
            }
            break;
        }

        printf("\nClient:\n%s\n", client_msg);

        if (is_exit_message(client_msg)) {
            break;
        }

        read_line("Reply:\n", reply, sizeof(reply));
        send_message(client_fd, reply);

        if (is_exit_message(reply)) {
            break;
        }
    }

    close(client_fd);
    printf("\nConnection closed.\n");
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
