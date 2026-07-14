# TCP Client-Server Chat (Phase 2)

An interactive TCP chat application written in C using the POSIX socket API. The client and server exchange multiple messages over a persistent connection until one side types `exit`.

Built on top of Phase 1 — same TCP fundamentals, now with a turn-based chat loop.

---

## Table of Contents

- [Quick Reference — Useful Commands](#quick-reference--useful-commands)
- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Running a Chat Session](#running-a-chat-session)
- [Example Session](#example-session)
- [How It Works](#how-it-works)
- [Code Walkthrough](#code-walkthrough)
- [Configuration](#configuration)
- [Error Handling](#error-handling)
- [Limitations (Phase 2)](#limitations-phase-2)
- [Troubleshooting](#troubleshooting)
- [Learning Objectives](#learning-objectives)

---

## Quick Reference — Useful Commands

All commands below assume you are inside the project directory in **WSL/Linux**.

### Enter the project (from Windows PowerShell)

```powershell
wsl
cd /mnt/e/coding/tcpserver
```

Adjust the path if your project lives elsewhere (e.g. `/mnt/c/Users/you/tcpserver`).

---

### Starting commands

**Build (first time, or after code changes):**

```bash
make
```

**Rebuild from scratch:**

```bash
make clean && make
```

**Start the server (Terminal 1 — leave this running):**

```bash
./server
```

**Start the client (Terminal 2 — open a second WSL terminal):**

```bash
wsl
cd /mnt/e/coding/tcpserver
./client
```

**Start server in background (optional):**

```bash
./server &
```

---

### During a chat session

| Action | Where | What to type |
|--------|-------|--------------|
| Send a message | Client terminal | Type your message at `Enter a message:` |
| Reply to client | Server terminal | Type your reply at `Reply:` |
| End the session | Either terminal | Type `exit` |

The client always sends first in each round. The server types each reply manually — responses are not auto-generated.

---

### Ending commands

**End the chat (recommended):**

Type `exit` in either the client or server terminal. The connection closes and both sides print `Connection closed.`

**Stop the server manually (Ctrl+C in server terminal):**

Press `Ctrl + C` in the terminal where `./server` is running.

**Kill server and client processes:**

```bash
pkill -f "./server"
pkill -f "./client"
```

**Free port 8080 (when you see `bind: Address already in use`):**

```bash
fuser -k 8080/tcp
pkill -f "./server"
pkill -f "./client"
```

> **Important:** Put a space after `-k` and `-f`:
> - Correct: `fuser -k 8080/tcp`
> - Wrong:   `fuser -k8080/tcp`

**Check if port 8080 is in use:**

```bash
ss -tlnp | grep 8080
```

No output means the port is free.

**Check running server/client processes:**

```bash
ps aux | grep -E '[./](server|client)'
```

---

### Full start-to-finish workflow

```bash
# 1. Go to project
cd /mnt/e/coding/tcpserver

# 2. Build
make

# 3. Terminal 1 — start server
./server

# 4. Terminal 2 — start client
./client

# 5. Chat back and forth, then type exit on either side

# 6. To stop everything afterward
pkill -f "./server"
pkill -f "./client"
```

---

## Overview

This project consists of two separate programs:

| Program    | Role   | Description                                              |
|------------|--------|----------------------------------------------------------|
| `server.c` | Server | Listens on port 8080, receives messages, types replies   |
| `client.c` | Client | Connects to the server, sends messages, displays replies |

Communication uses **IPv4**, **TCP** (`SOCK_STREAM`), and **localhost** (`127.0.0.1:8080`).

---

## Features

- Persistent TCP connection for multiple message exchanges
- Interactive turn-based chat (client sends, server replies)
- Server user types every reply through the terminal
- Graceful shutdown with the `exit` command
- Server accepts new clients after a session ends (no restart needed)
- Error checking on every system call with `perror()`
- Clean helper-function structure with no global variables

---

## Project Structure

```
tcpserver/
├── server.c      # TCP server — accept loop + chat handler
├── client.c      # TCP client — connect + chat loop
├── Makefile      # Build rules (gcc, -Wall -Wextra)
└── README.md     # This file
```

After building:

```
server    # Run in Terminal 1
client    # Run in Terminal 2
```

---

## Requirements

### Platform

POSIX socket APIs (`<sys/socket.h>`, `<arpa/inet.h>`, `<unistd.h>`). Run on:

- **Linux** (native)
- **WSL** (Windows Subsystem for Linux) — recommended on Windows
- **macOS** (POSIX-compatible)

Native Windows compilers (e.g. MinGW) are **not** supported.

### Toolchain

| Tool   | Purpose          |
|--------|------------------|
| `gcc`  | C compiler       |
| `make` | Build automation |

Install on Ubuntu/Debian (WSL):

```bash
sudo apt update
sudo apt install build-essential
```

---

## Building

```bash
make
```

Individual targets:

```bash
make server    # Build only the server
make client    # Build only the client
make clean     # Remove compiled binaries
```

Manual compilation:

```bash
gcc -Wall -Wextra -o server server.c
gcc -Wall -Wextra -o client client.c
```

---

## Running a Chat Session

You need **two terminals**. Start the server first.

### Terminal 1 — Server

```bash
wsl
cd /mnt/e/coding/tcpserver
./server
```

Expected output:

```
Server started...
Listening on port 8080...
```

The server blocks on `accept()` until a client connects.

### Terminal 2 — Client

```bash
wsl
cd /mnt/e/coding/tcpserver
./client
```

Type messages when prompted. The connection stays open for multiple exchanges.

### Ending the session

Type `exit` on either side:

- **Client types `exit`** — sends it, closes immediately, no reply expected
- **Server types `exit`** at the `Reply:` prompt — sends it to the client, both close

After a session ends, the server prints `Waiting for another client...` and accepts the next connection without restarting.

---

## Example Session

### Terminal 1 (Server)

```
$ ./server

Server started...
Listening on port 8080...

Client connected.

Client:
Hello

Reply:
Hi! Welcome.

Client:
How are you?

Reply:
I'm good!

Client:
exit

Connection closed.

Waiting for another client...
```

### Terminal 2 (Client)

```
$ ./client

Enter a message:
Hello

Server:
Hi! Welcome.
Enter a message:
How are you?

Server:
I'm good!
Enter a message:
exit

Connection closed.
```

---

## How It Works

### Conversation flow

```
  CLIENT                          SERVER
    |                                |
    |         TCP connect            |
    |------------------------------->|
    |                                | accept()
    |                                |
    |  +-------- chat loop --------+  |
    |  | send(message)             |  |
    |  |-------------------------->|  | recv() + print "Client:"
    |  |                           |  | read reply from stdin
    |  | recv() + print "Server:"  |  |
    |  |<--------------------------|  | send(reply)
    |  +-------- until exit ------+  |
    |                                |
    |         close()                |
    |------------------------------->|
    |                                | close(client_fd)
    |                                | loop → accept() again
```

### Server lifecycle

1. **`socket()` / `bind()` / `listen()`** — Set up listening socket on port 8080
2. **`accept()`** — Wait for a client connection
3. **Chat loop:**
   - **`recv()`** — Read client message
   - Print `Client:` + message
   - If message is `exit`, break
   - Prompt `Reply:` and read from stdin
   - **`send()`** — Send reply to client
   - If reply is `exit`, break
4. **`close()`** — Close client socket
5. Return to step 2 for the next client

### Client lifecycle

1. **`socket()` / `connect()`** — Connect to `127.0.0.1:8080`
2. **Chat loop:**
   - Prompt user, read message from stdin
   - **`send()`** — Send message to server
   - If message is `exit`, break
   - **`recv()`** — Read server reply
   - Print `Server:` + reply
   - If reply is `exit`, break
3. **`close()`** — Close connection and exit

---

## Code Walkthrough

### `server.c`

| Function                    | Responsibility                                           |
|-----------------------------|----------------------------------------------------------|
| `create_listening_socket()` | Creates socket, binds to port 8080, calls `listen()`     |
| `handle_client()`           | Chat loop: recv → display → prompt reply → send          |
| `recv_message()`            | Wrapper around `recv()` with null-termination            |
| `send_message()`            | Wrapper around `send()` with error checking              |
| `read_line()`               | Reads a line from stdin with a custom prompt             |
| `is_exit_message()`         | Checks if a message equals `"exit"`                      |
| `main()`                    | Startup messages, infinite accept loop                   |

### `client.c`

| Function                    | Responsibility                                           |
|-----------------------------|----------------------------------------------------------|
| `create_connected_socket()` | Creates socket and connects to the server                |
| `read_user_message()`       | Prompts user, reads stdin, strips trailing newline       |
| `recv_message()`            | Wrapper around `recv()` with null-termination            |
| `send_message()`            | Wrapper around `send()` with error checking              |
| `is_exit_message()`         | Checks if a message equals `"exit"`                      |
| `main()`                    | Connect, chat loop, close                                |

Key constants (both files):

```c
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define EXIT_CMD    "exit"
```

---

## Configuration

| Constant      | Value       | File       | Description                |
|---------------|-------------|------------|----------------------------|
| `SERVER_PORT` | `8080`      | Both       | TCP port number            |
| `SERVER_IP`   | `127.0.0.1` | `client.c` | Server address (localhost) |
| `BUFFER_SIZE` | `1024`      | Both       | Max message size (bytes)   |
| `EXIT_CMD`    | `"exit"`    | Both       | Command to end the session |

To change the port, update `SERVER_PORT` in **both** files, then run `make clean && make`.

---

## Error Handling

Every system call return value is checked. On failure:

1. Prints a descriptive message via `perror()`
2. Closes open file descriptors
3. Exits gracefully (client) or returns to the accept loop (server)

| Situation                    | Behavior                                              |
|------------------------------|-------------------------------------------------------|
| `recv()` returns `0`         | Peer disconnected; print message and close            |
| `recv()` returns `-1`        | Print error; server returns to accept loop            |
| Client sends `exit`          | Server displays it, closes without prompting a reply  |
| Server sends `exit` as reply | Client displays it and closes                         |
| `bind()` fails (port in use) | See [Troubleshooting](#troubleshooting)               |

---

## Limitations (Phase 2)

Intentionally out of scope:

- Multithreading or concurrent clients
- Multiple simultaneous connections
- Authentication or encryption (TLS/SSL)
- File transfer or message history
- Command-line argument parsing
- Configuration files

These may be added in future phases.

---

## Troubleshooting

### `bind: Address already in use`

A previous server or stuck client is holding port 8080.

```bash
fuser -k 8080/tcp
pkill -f "./server"
pkill -f "./client"
ss -tlnp | grep 8080    # should show nothing
./server
```

Make sure there is a **space** between `-k` and `8080` in the `fuser` command.

### `connect: Connection refused`

- Start `./server` in Terminal 1 before running `./client`
- Verify port 8080 is not blocked by a firewall

### Server starts but immediately shows `bind: Address already in use`

The startup messages print **before** `bind()` is called. If bind fails, the server exits even though you saw "Listening on port 8080...". Free the port with the commands above.

### `make: command not found` or `gcc: command not found`

```bash
sudo apt update && sudo apt install build-essential
```

### Building on native Windows

Use WSL:

```powershell
wsl
cd /mnt/e/coding/tcpserver
make
./server
```

### Message truncated

Messages longer than 1023 characters are truncated by a single `recv()` call. Increase `BUFFER_SIZE` in both files if needed.

---

## Learning Objectives

After working through this project, you should understand:

- Persistent TCP connections vs. one-shot exchanges
- Turn-based client-server communication using loops
- Interactive terminal input on both client and server sides
- Graceful connection shutdown with an application-level `exit` command
- The TCP connection lifecycle: connect → exchange data → close
- Socket creation, binding, listening, accepting, connecting
- Sending and receiving data with `send()` and `recv()`
- `struct sockaddr_in` and address conversion (`htons`, `inet_pton`)
- Error handling with `perror()` in systems programming

---

## License

This project is provided for educational purposes.
