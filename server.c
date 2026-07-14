#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define EXIT_CMD    "exit"

static pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;
static int next_client_id = 1;
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_fd;
    int client_id;
} client_context_t;

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

static int send_message(int fd, const char *msg)
{
    if (send(fd, msg, strlen(msg), 0) == -1) {
        perror("send");
        return -1;
    }
    return 0;
}

static int assign_client_id(void)
{
    int id;

    pthread_mutex_lock(&id_mutex);
    id = next_client_id++;
    pthread_mutex_unlock(&id_mutex);
    return id;
}

static void print_client_message(int client_id, const char *msg)
{
    pthread_mutex_lock(&io_mutex);
    printf("\n[Client %d]\n%s\n", client_id, msg);
    pthread_mutex_unlock(&io_mutex);
}

static int read_line_safe(const char *prompt, char *buf, size_t size)
{
    pthread_mutex_lock(&io_mutex);
    printf("%s", prompt);

    if (fgets(buf, (int)size, stdin) == NULL) {
        pthread_mutex_unlock(&io_mutex);
        fprintf(stderr, "Failed to read input.\n");
        return -1;
    }

    strip_newline(buf);
    pthread_mutex_unlock(&io_mutex);
    return 0;
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

static void *client_thread(void *arg)
{
    client_context_t *ctx = (client_context_t *)arg;
    char client_msg[BUFFER_SIZE];
    char reply[BUFFER_SIZE];
    ssize_t bytes_received;

    /* Thread lifecycle: receive messages, prompt for replies, exit on disconnect/exit. */
    while (1) {
        bytes_received = recv_message(ctx->client_fd, client_msg, sizeof(client_msg));
        if (bytes_received <= 0) {
            break;
        }

        print_client_message(ctx->client_id, client_msg);

        if (is_exit_message(client_msg)) {
            break;
        }

        if (read_line_safe("Reply:\n", reply, sizeof(reply)) == -1) {
            break;
        }
        if (send_message(ctx->client_fd, reply) == -1) {
            break;
        }
        if (is_exit_message(reply)) {
            break;
        }
    }

    close(ctx->client_fd);
    pthread_mutex_lock(&io_mutex);
    printf("\n[Client %d] Connection closed.\n", ctx->client_id);
    pthread_mutex_unlock(&io_mutex);

    free(ctx);
    return NULL;
}

int main(void)
{
    int server_fd;
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    client_context_t *ctx;
    pthread_t tid;

    printf("Server started...\n");
    printf("Listening on port %d...\n", SERVER_PORT);

    server_fd = create_listening_socket();

    while (1) {
        client_addr_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        ctx = malloc(sizeof(client_context_t));
        if (ctx == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        ctx->client_fd = client_fd;
        ctx->client_id = assign_client_id();

        printf("\nClient #%d connected.\n", ctx->client_id);

        if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(ctx);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
