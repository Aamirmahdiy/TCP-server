# TCP Client-Server (Phase 1)

A minimal TCP client-server application written in C using the POSIX socket API. The project demonstrates the fundamentals of socket programming and TCP communication on Linux — one message per connection, with the server accepting clients sequentially.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Running](#running)
- [Example Session](#example-session)
- [How It Works](#how-it-works)
- [Code Walkthrough](#code-walkthrough)
- [Configuration](#configuration)
- [Error Handling](#error-handling)
- [Limitations (Phase 1)](#limitations-phase-1)
- [Troubleshooting](#troubleshooting)
- [Learning Objectives](#learning-objectives)

---

## Overview

This project consists of two separate programs:

| Program    | Role   | Description                                      |
|------------|--------|--------------------------------------------------|
| `server.c` | Server | Listens on port 8080, receives messages, replies  |
| `client.c` | Client | Connects to the server, sends a user message     |

Communication uses **IPv4**, **TCP** (`SOCK_STREAM`), and **localhost** (`127.0.0.1:8080`).

---

## Features

- TCP socket creation, binding, listening, and accepting (server)
- TCP socket creation and connection (client)
- Interactive message input from the user (client)
- Server prints received messages and sends a confirmation reply
- Server loop: after each client disconnects, it waits for the next connection
- Error checking on every system call with `perror()`
- Clean helper-function structure with no global variables

---

## Project Structure

```
tcpserver/
├── server.c      # TCP server implementation
├── client.c      # TCP client implementation
├── Makefile      # Build rules (gcc, -Wall -Wextra)
└── README.md     # This file
```

After building, two executables are produced in the project root:

```
server    # Run in one terminal
client    # Run in another terminal
```

---

## Requirements

### Platform

This project uses POSIX socket APIs (`<sys/socket.h>`, `<arpa/inet.h>`, `<unistd.h>`). It must be compiled and run on:

- **Linux** (native)
- **WSL** (Windows Subsystem for Linux) — recommended on Windows
- **macOS** (POSIX-compatible)

Native Windows compilers (e.g. MinGW) are **not** supported because they do not provide the same POSIX socket headers.

### Toolchain

| Tool   | Purpose              |
|--------|----------------------|
| `gcc`  | C compiler           |
| `make` | Build automation     |

On Ubuntu/Debian (including WSL):

```bash
sudo apt update
sudo apt install build-essential
```

On Fedora:

```bash
sudo dnf install gcc make
```

---

## Building

Clone or navigate to the project directory, then build both programs:

```bash
make
```

This compiles with `-Wall -Wextra` to catch common warnings.

### Individual targets

```bash
make server    # Build only the server
make client    # Build only the client
make clean     # Remove compiled binaries
```

### Manual compilation

If you prefer not to use `make`:

```bash
gcc -Wall -Wextra -o server server.c
gcc -Wall -Wextra -o client client.c
```

---

## Running

You need **two terminals**. Start the server first, then run the client.

### Terminal 1 — Start the server

```bash
./server
```

Expected output:

```
Server started...
Listening on port 8080...
```

The server now blocks on `accept()`, waiting for a client.

### Terminal 2 — Run the client

```bash
./client
```

Type a message when prompted and press Enter. The client sends it to the server, waits for the reply, prints it, and exits.

### Multiple clients (sequential)

After one client finishes, the server prints `Waiting for another client...` and accepts the next connection. Run `./client` again in Terminal 2 — no need to restart the server.

---

## Example Session

### Terminal 1 (Server)

```
$ ./server

Server started...
Listening on port 8080...

Client connected.

Received:
Hello Server

Sending confirmation...

Waiting for another client...
```

### Terminal 2 (Client)

```
$ ./client

Enter a message:
Hello Server

Server replied:
Server received your message successfully.
```

---

## How It Works

### Connection flow

```
  CLIENT                          SERVER
    |                                |
    |         TCP connect            |
    |------------------------------->|
    |                                | accept()
    |                                |
    |         send(message)          |
    |------------------------------->|
    |                                | recv() + print
    |                                |
    |    send(confirmation)          |
    |<-------------------------------|
    | recv() + print                 |
    |                                |
    |         close()                |
    |------------------------------->|
    |                                | close(client_fd)
    |                                | loop → accept() again
```

### TCP lifecycle (server)

1. **`socket()`** — Create a TCP socket (`AF_INET`, `SOCK_STREAM`)
2. **`bind()`** — Attach the socket to port 8080 on all interfaces (`INADDR_ANY`)
3. **`listen()`** — Mark the socket as passive; queue up to 5 pending connections
4. **`accept()`** — Block until a client connects; return a new socket for that client
5. **`recv()`** — Read the client's message into a buffer
6. **`send()`** — Send the confirmation string back
7. **`close()`** — Close the client socket; loop back to step 4

### TCP lifecycle (client)

1. **`socket()`** — Create a TCP socket
2. **`connect()`** — Connect to `127.0.0.1:8080`
3. Read user input from stdin (`fgets`)
4. **`send()`** — Send the message to the server
5. **`recv()`** — Read the server's response
6. **`close()`** — Close the connection and exit

---

## Code Walkthrough

### `server.c`

| Function                    | Responsibility                                      |
|-----------------------------|-----------------------------------------------------|
| `create_listening_socket()` | Creates socket, binds to port 8080, calls `listen()` |
| `handle_client()`           | Receives message, prints it, sends confirmation, closes client fd |
| `main()`                    | Prints startup info, runs infinite accept loop      |

Key constants:

```c
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define CONFIRMATION_MSG "Server received your message successfully.\n"
```

The server binds to `INADDR_ANY`, so it accepts connections on all network interfaces. The client connects specifically to `127.0.0.1` (localhost).

### `client.c`

| Function                    | Responsibility                                      |
|-----------------------------|-----------------------------------------------------|
| `create_connected_socket()` | Creates socket and connects to the server           |
| `read_user_message()`       | Prompts user, reads stdin, strips trailing newline  |
| `main()`                    | Orchestrates connect → send → recv → close          |

The client uses `inet_pton()` to convert the IP string `"127.0.0.1"` into binary form for `struct sockaddr_in`.

---

## Configuration

These values are defined as constants at the top of each source file:

| Constant       | Value       | File       | Description                |
|----------------|-------------|------------|----------------------------|
| `SERVER_PORT`  | `8080`      | Both       | TCP port number            |
| `SERVER_IP`    | `127.0.0.1` | `client.c` | Server address (localhost) |
| `BUFFER_SIZE`  | `1024`      | Both       | Max message size (bytes)   |

To change the port, update `SERVER_PORT` in **both** `server.c` and `client.c`, then rebuild with `make clean && make`.

---

## Error Handling

Every system call return value is checked. On failure, the program:

1. Prints a descriptive message via `perror()`
2. Closes any open file descriptors
3. Exits with `EXIT_FAILURE`

Example pattern used throughout:

```c
if (result == -1) {
    perror("operation_name");
    exit(EXIT_FAILURE);
}
```

Additional cases handled:

| Situation                          | Behavior                                              |
|------------------------------------|-------------------------------------------------------|
| `recv()` returns `0`               | Peer closed connection; print message and exit/return |
| `inet_pton()` fails                | Invalid IP address; print error and exit              |
| `fgets()` returns `NULL`         | Input read failure; print error and exit              |
| `bind()` fails (port in use)       | `perror("bind")` — see [Troubleshooting](#troubleshooting) |

---

## Limitations (Phase 1)

This is an educational project. The following are **intentionally out of scope**:

- Multithreading or concurrent clients
- Multiple simultaneous connections
- Authentication or encryption (TLS/SSL)
- File transfer
- Persistent chat sessions
- Command-line argument parsing
- Configuration files
- Partial send/recv loops (messages are assumed to fit in one buffer)

These may be added in future phases.

---

## Troubleshooting

### `bind: Address already in use`

Another process (often a previous server instance) is using port 8080.

**Find and kill the process (Linux/WSL):**

```bash
# Find what's using port 8080
ss -tlnp | grep 8080

# Kill by port
fuser -k 8080/tcp

# Or kill by name
pkill -f "./server"
```

Then start the server again.

### `connect: Connection refused`

The client cannot reach the server. Common causes:

- The server is not running — start `./server` first
- Wrong port or IP — verify constants match in both files
- Firewall blocking localhost (uncommon on Linux)

### `make: command not found` or `gcc: command not found`

The build toolchain is not installed. See [Requirements](#requirements).

**On WSL (Ubuntu):**

```bash
sudo apt update && sudo apt install build-essential
```

### Building on native Windows

POSIX headers (`<arpa/inet.h>`, `<unistd.h>`) are not available in standard Windows toolchains. Use **WSL**:

```powershell
wsl
cd /mnt/e/coding/tcpserver   # adjust path to your project location
make
./server
```

### Message truncated

Messages longer than `BUFFER_SIZE - 1` (1023 characters) will be truncated by a single `recv()` call. Increase `BUFFER_SIZE` in both files if needed, or implement a read loop in a future phase.

---

## Learning Objectives

After working through this project, you should understand:

- What TCP is and how it differs from UDP
- The client-server model in network programming
- How to create, bind, listen, and accept sockets (server side)
- How to create and connect sockets (client side)
- How to send and receive data with `send()` and `recv()`
- The role of `struct sockaddr_in` and address conversion (`htons`, `inet_pton`)
- Why the server uses two sockets: one for listening, one per client
- Basic error handling with `perror()` in systems programming
- The TCP connection lifecycle: connect → exchange data → close

---

## License

This project is provided for educational purposes.
