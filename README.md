# TCP Client-Server Chat (Phase 3)

A multi-client TCP chat application written in C using the POSIX socket API and POSIX threads. Multiple clients can connect simultaneously; the server receives all messages and lets the operator choose which client to reply to using simple commands.

---

## Table of Contents

- [Quick Reference — Useful Commands](#quick-reference--useful-commands)
- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Running a Chat Session](#running-a-chat-session)
- [Server Commands](#server-commands)
- [Example Session](#example-session)
- [How It Works](#how-it-works)
- [Code Walkthrough](#code-walkthrough)
- [Configuration](#configuration)
- [Error Handling](#error-handling)
- [Limitations](#limitations)
- [Troubleshooting](#troubleshooting)
- [Project Phases](#project-phases)
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

**Start a client (Terminal 2, 3, 4… — one client per terminal):**

```bash
wsl
cd /mnt/e/coding/tcpserver
./client
```

You can run **multiple clients at the same time** in separate terminals.

---

### During a chat session

| Action | Where | What to type |
|--------|-------|--------------|
| Send a message | Client terminal | Type at `Enter a message:` |
| Reply to a specific client | Server terminal | `to client1: hi` |
| Close a specific client | Server terminal | `toclient2:exit` |
| End from client side | Client terminal | `exit` |

**Client numbers** are assigned in connect order: first client is `#1`, second is `#2`, etc.

---

### Server reply commands

```text
to client1: hi
to client 2: Hello there!
toclient2:exit
```

All of these formats work:

| Format | Example |
|--------|---------|
| `to client<N>: <message>` | `to client1: hi` |
| `to client <N>: <message>` | `to client 1: hi` |
| `toclient<N>:<message>` | `toclient2:exit` |

Sending `exit` (case-insensitive) closes that client's connection.

---

### Ending commands

**Close one client from the server:**

```text
toclient1:exit
```

**Close from the client side:**

Type `exit` at `Enter a message:` in the client terminal.

**Stop the server (Ctrl+C in server terminal):**

Press `Ctrl + C` in the terminal where `./server` is running.

**Kill all server and client processes:**

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

# 4. Terminal 2 — start client 1
./client

# 5. Terminal 3 — start client 2 (optional)
./client

# 6. Clients send messages; server replies with:
#    to client1: hi
#    to client2: hello

# 7. Close a client from server:
#    toclient1:exit

# 8. Stop everything afterward
pkill -f "./server"
pkill -f "./client"
```

---

## Overview

This project consists of two separate programs:

| Program    | Role   | Description                                                       |
|------------|--------|-------------------------------------------------------------------|
| `server.c` | Server | Listens on port 8080, handles multiple clients with threads       |
| `client.c` | Client | Connects to the server, sends messages, displays replies          |

Communication uses **IPv4**, **TCP** (`SOCK_STREAM`), **POSIX threads** (`pthread`), and **localhost** (`127.0.0.1:8080`).

---

## Features

- **Multi-client support** — multiple clients connect simultaneously
- **Thread-per-client** — each client gets its own dedicated thread
- **Non-blocking message display** — all client messages appear immediately; no need to reply before seeing the next message
- **Choose who to reply to** — server operator sends targeted replies with `to client<N>: <message>`
- **Persistent TCP connections** — clients stay connected for multiple message exchanges
- **Graceful shutdown** — `exit` from client or `toclient<N>:exit` from server
- **Client registry** — tracks active connections by client ID
- Error checking on every system call with `perror()`

---

## Project Structure

```
tcpserver/
├── server.c      # Multi-client TCP server with pthread + command dispatch
├── client.c      # TCP client — connect + chat loop
├── Makefile      # Build rules (gcc, -Wall -Wextra, -pthread)
└── README.md     # This file
```

After building:

```
server    # Run in Terminal 1
client    # Run in one or more additional terminals
```

---

## Requirements

### Platform

POSIX socket APIs and POSIX threads. Run on:

- **Linux** (native)
- **WSL** (Windows Subsystem for Linux) — recommended on Windows
- **macOS** (POSIX-compatible)

Native Windows compilers (e.g. MinGW) are **not** supported.

### Toolchain

| Tool     | Purpose              |
|----------|----------------------|
| `gcc`    | C compiler           |
| `make`   | Build automation     |
| `pthread`| POSIX threads (linked with `-pthread`) |

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
make server    # Build only the server (links with -pthread)
make client    # Build only the client
make clean     # Remove compiled binaries
```

Manual compilation:

```bash
gcc -Wall -Wextra -pthread -o server server.c
gcc -Wall -Wextra -o client client.c
```

---

## Running a Chat Session

You need **at least two terminals** — one for the server, one or more for clients.

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
Commands: to client<N>: <message>
Examples: to client1: hi   |   toclient2:exit
```

### Terminal 2 (and 3, 4…) — Clients

```bash
wsl
cd /mnt/e/coding/tcpserver
./client
```

Each client connects independently. Type messages when prompted.

---

## Server Commands

The server operator types commands in the **server terminal** to send messages to specific clients.

### Syntax

```text
to client<N>: <message>
```

### Examples

| Command | Effect |
|---------|--------|
| `to client1: hi` | Sends `hi` to Client 1 |
| `to client 2: Hello!` | Sends `Hello!` to Client 2 |
| `toclient1:exit` | Sends `exit` to Client 1 and closes that connection |
| `to client3: See you later` | Sends message to Client 3 |

### How messages appear

When any client sends a message, it shows up immediately on the server:

```text
[Client 1]
Hello

[Client 2]
Hi
```

You can reply to Client 2 first even if Client 1 sent a message earlier:

```text
to client2: Welcome!
Sent to [Client 2]: Welcome!

to client1: Hi there!
Sent to [Client 1]: Hi there!
```

---

## Example Session

### Terminal 1 (Server)

```
$ ./server

Server started...
Listening on port 8080...
Commands: to client<N>: <message>
Examples: to client1: hi   |   toclient2:exit

Client #1 connected.

Client #2 connected.

[Client 1]
Hello

[Client 2]
Hi

to client2: Welcome!
Sent to [Client 2]: Welcome!

to client1: Hi there!
Sent to [Client 1]: Hi there!

toclient2:exit
Sent to [Client 2]: exit
[Client 2] Connection closed.

[Client 1]
exit

[Client 1] Connection closed.
```

### Terminal 2 (Client 1)

```
$ ./client

Enter a message:
Hello

Server:
Hi there!
Enter a message:
exit

Connection closed.
```

### Terminal 3 (Client 2)

```
$ ./client

Enter a message:
Hi

Server:
Welcome!
Enter a message:

Server:
exit

Connection closed.
```

---

## How It Works

### Architecture

```
  MAIN THREAD              CLIENT THREADS           COMMAND THREAD
       |                        |                         |
  accept() loop            recv + display              read stdin
       |                        |                         |
  spawn thread ──────────> [Client 1 thread]             |
  spawn thread ──────────> [Client 2 thread]      parse "to clientN: msg"
  spawn thread ──────────> [Client 3 thread]      send to client N's socket
```

- **Main thread** — accepts new connections and spawns a thread for each client
- **Client threads** — receive messages and display them; never block on replies
- **Command thread** — reads server operator commands and sends replies to the chosen client

### Conversation flow

```
  CLIENT 1          CLIENT 2              SERVER
     |                 |                     |
     |--- connect ---->|                     | accept → thread 1
     |                 |--- connect --------->| accept → thread 2
     |                 |                     |
     |--- "Hello" ---->|-------------------->| [Client 1] Hello
     |                 |--- "Hi" ----------->| [Client 2] Hi
     |                 |                     |
     |                 |    operator types: to client2: Welcome!
     |                 |<--------------------|
     |                 |                     |
     |    operator types: to client1: Hi there!
     |<-------------------------------------|
```

### Client lifecycle (unchanged from Phase 2)

1. **`connect()`** to `127.0.0.1:8080`
2. **Chat loop:** send message → wait for reply → repeat
3. Type `exit` to disconnect

---

## Code Walkthrough

### `server.c`

| Function / Thread | Responsibility |
|-------------------|----------------|
| `main()` | Accept loop, spawns client threads and command thread |
| `client_thread()` | Receive-only loop: recv → display `[Client N]` message |
| `command_thread()` | Read stdin commands → parse → send to target client |
| `parse_command()` | Parses `to client1: hi` and `toclient2:exit` formats |
| `send_to_client()` | Looks up client by ID in registry, sends message |
| `register_client()` | Adds client to active registry |
| `disconnect_client_by_id()` | Closes socket and marks client inactive |
| `create_listening_socket()` | Socket setup: bind port 8080, listen |

### `client.c`

| Function | Responsibility |
|----------|----------------|
| `create_connected_socket()` | Creates socket and connects to the server |
| `read_user_message()` | Prompts user, reads stdin, strips trailing newline |
| `main()` | Connect, chat loop, close |

Key constants:

```c
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define EXIT_CMD    "exit"
#define MAX_CLIENTS 32        /* server.c only */
```

---

## Configuration

| Constant      | Value       | File       | Description                    |
|-----------------|-------------|------------|--------------------------------|
| `SERVER_PORT`   | `8080`      | Both       | TCP port number                |
| `SERVER_IP`     | `127.0.0.1` | `client.c` | Server address (localhost)     |
| `BUFFER_SIZE`   | `1024`      | Both       | Max message size (bytes)       |
| `EXIT_CMD`      | `"exit"`    | Both       | Command to end a session       |
| `MAX_CLIENTS`   | `32`        | `server.c` | Max simultaneous connections   |

To change the port, update `SERVER_PORT` in **both** files, then run `make clean && make`.

---

## Error Handling

Every system call return value is checked. On failure:

1. Prints a descriptive message via `perror()`
2. Closes open file descriptors
3. Exits gracefully (client) or continues serving other clients (server)

| Situation | Behavior |
|-----------|----------|
| `recv()` returns `0` | Peer disconnected; thread closes that client |
| Client sends `exit` | Thread displays message and closes connection |
| Server sends `toclient<N>:exit` | Client receives `exit` and closes |
| Unknown server command | Prints usage hint, continues |
| Client ID not connected | Prints error, continues |
| `pthread_create` fails | Closes socket, continues accept loop |
| `bind()` fails (port in use) | See [Troubleshooting](#troubleshooting) |

---

## Limitations

Intentionally out of scope:

- Chat rooms or broadcasting to all clients
- Authentication or encryption (TLS/SSL)
- File transfer or message history
- Thread pool (uses thread-per-client instead)
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

### Server shows startup messages then `bind: Address already in use`

The startup messages print **before** `bind()` is called. If bind fails, the server exits even though you saw "Listening on port 8080...". Free the port with the commands above.

### `Client N is not connected`

The client has already disconnected or the client number is wrong. Client numbers are assigned in connect order and never reused during a server session.

### `Unknown command. Use: to client<N>: <message>`

Check the command format. Valid examples: `to client1: hi`, `to client 2: hello`, `toclient1:exit`.

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

## Project Phases

| Phase | Feature |
|-------|---------|
| **Phase 1** | Basic TCP — one message per connection, auto-confirmation reply |
| **Phase 2** | Persistent chat — multiple messages, interactive server replies, `exit` command |
| **Phase 3** | Multi-client — POSIX threads, simultaneous clients, command-based replies (`to client<N>:`) |

---

## Learning Objectives

After working through this project, you should understand:

- Multi-client TCP server architecture
- Concurrent programming with POSIX threads (`pthread`)
- Thread-per-client design pattern
- Separating receive and send responsibilities across threads
- Concurrent socket communication without blocking other clients
- Proper thread cleanup and connection lifecycle management
- Persistent TCP connections and graceful shutdown
- Socket creation, binding, listening, accepting, connecting
- Sending and receiving data with `send()` and `recv()`
- Error handling with `perror()` in systems programming

---

## License

This project is provided for educational purposes.
