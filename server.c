#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define EXIT_CMD    "exit"
#define MAX_CLIENTS 32

static pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static int next_client_id = 1;
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_fd;
    int client_id;
    int active;
} client_context_t;

static client_context_t *client_registry[MAX_CLIENTS];
static int registry_count = 0;

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

static int is_exit_command(const char *msg)
{
    return strcasecmp(msg, EXIT_CMD) == 0;
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
    fflush(stdout);
    pthread_mutex_unlock(&io_mutex);
}

static int register_client(client_context_t *ctx)
{
    pthread_mutex_lock(&registry_mutex);
    if (registry_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&registry_mutex);
        return -1;
    }
    client_registry[registry_count++] = ctx;
    pthread_mutex_unlock(&registry_mutex);
    return 0;
}

static void unregister_client(client_context_t *ctx)
{
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < registry_count; i++) {
        if (client_registry[i] == ctx) {
            client_registry[i] = client_registry[registry_count - 1];
            registry_count--;
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
}

static int send_to_client(int client_id, const char *message)
{
    int fd = -1;
    int active = 0;

    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < registry_count; i++) {
        if (client_registry[i]->client_id == client_id) {
            fd = client_registry[i]->client_fd;
            active = client_registry[i]->active;
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);

    if (!active || fd == -1) {
        return -1;
    }

    return send_message(fd, message);
}

static void disconnect_client_by_id(int client_id)
{
    pthread_mutex_lock(&registry_mutex);
    for (int i = 0; i < registry_count; i++) {
        if (client_registry[i]->client_id == client_id && client_registry[i]->active) {
            client_registry[i]->active = 0;
            if (client_registry[i]->client_fd != -1) {
                close(client_registry[i]->client_fd);
                client_registry[i]->client_fd = -1;
            }
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
}


static int parse_command(const char *line, int *client_id, char *message, size_t msg_size)
{
    char line_copy[BUFFER_SIZE];
    char *p;
    int id = 0;

    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    strip_newline(line_copy);
    p = line_copy;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (strncasecmp(p, "to", 2) == 0) {
        p += 2;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
    }

    if (strncasecmp(p, "client", 6) != 0) {
        return -1;
    }
    p += 6;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (*p < '0' || *p > '9') {
        return -1;
    }

    while (*p >= '0' && *p <= '9') {
        id = id * 10 + (*p - '0');
        p++;
    }

    if (id <= 0) {
        return -1;
    }

    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p != ':') {
        return -1;
    }
    p++;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    strncpy(message, p, msg_size - 1);
    message[msg_size - 1] = '\0';
    *client_id = id;
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
    ssize_t bytes_received;
    int was_active;

    while (1) {
        bytes_received = recv_message(ctx->client_fd, client_msg, sizeof(client_msg));
        if (bytes_received <= 0) {
            break;
        }

        print_client_message(ctx->client_id, client_msg);

        if (is_exit_message(client_msg)) {
            break;
        }
    }

    pthread_mutex_lock(&registry_mutex);
    was_active = ctx->active;
    if (ctx->active) {
        ctx->active = 0;
        if (ctx->client_fd != -1) {
            close(ctx->client_fd);
            ctx->client_fd = -1;
        }
    }
    pthread_mutex_unlock(&registry_mutex);

    if (was_active) {
        pthread_mutex_lock(&io_mutex);
        printf("\n[Client %d] Connection closed.\n", ctx->client_id);
        fflush(stdout);
        pthread_mutex_unlock(&io_mutex);
    }

    free(ctx);
    return NULL;
}

static void *command_thread(void *arg)
{
    char line[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int client_id;

    (void)arg;

    pthread_mutex_lock(&io_mutex);
    printf("Commands: to client<N>: <message>\n");
    printf("Examples: to client1: hi   |   toclient2:exit\n");
    fflush(stdout);
    pthread_mutex_unlock(&io_mutex);

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (parse_command(line, &client_id, message, sizeof(message)) != 0) {
            pthread_mutex_lock(&io_mutex);
            fprintf(stderr, "Unknown command. Use: to client<N>: <message>\n");
            fflush(stdout);
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        if (send_to_client(client_id, message) == -1) {
            pthread_mutex_lock(&io_mutex);
            fprintf(stderr, "Client %d is not connected.\n", client_id);
            fflush(stdout);
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        pthread_mutex_lock(&io_mutex);
        printf("Sent to [Client %d]: %s\n", client_id, message);
        fflush(stdout);
        pthread_mutex_unlock(&io_mutex);

        if (is_exit_command(message)) {
            disconnect_client_by_id(client_id);
            pthread_mutex_lock(&io_mutex);
            printf("[Client %d] Connection closed.\n", client_id);
            fflush(stdout);
            pthread_mutex_unlock(&io_mutex);
        }
    }

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
    pthread_t cmd_tid;

    printf("Server started...\n");
    printf("Listening on port %d...\n", SERVER_PORT);

    server_fd = create_listening_socket();

    if (pthread_create(&cmd_tid, NULL, command_thread, NULL) != 0) {
        perror("pthread_create");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    pthread_detach(cmd_tid);

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
        ctx->active = 1;

        if (register_client(ctx) == -1) {
            fprintf(stderr, "Maximum number of clients reached.\n");
            close(client_fd);
            free(ctx);
            continue;
        }

        if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
            perror("pthread_create");
            unregister_client(ctx);
            close(client_fd);
            free(ctx);
            continue;
        }

        pthread_detach(tid);

        printf("\nClient #%d connected.\n", ctx->client_id);
        fflush(stdout);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
