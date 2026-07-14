# TCP Client-Server Chat (Phase 3)

Multi-client TCP chat in C using POSIX sockets and pthread. The server listens on port 8080; multiple clients can connect at once. The server operator replies to specific clients with commands like `to client1: hi`.

**Requires:** Linux, WSL, or macOS — not native Windows.

---

## Quick Start

```powershell
wsl
cd /mnt/e/coding/tcpserver
```

```bash
make
./server          # Terminal 1
./client          # Terminal 2, 3, … (one client per terminal)
```

Install toolchain if needed:

```bash
sudo apt update && sudo apt install build-essential
```

---

## Server Commands

Reply to a specific client from the server terminal:

```text
to client1: hi
to client 2: hello
toclient2:exit
```

| Action | Command |
|--------|---------|
| Send a message to Client 1 | `to client1: your message` |
| Close Client 2 from server | `toclient2:exit` |
| Close from client side | type `exit` in the client terminal |

Client numbers are assigned in connect order (`#1`, `#2`, …). All client messages appear immediately — you choose who to reply to.

---

## Stop / Cleanup

```bash
# Stop server: Ctrl+C in the server terminal

# Kill all processes
pkill -f "./server"
pkill -f "./client"

# Free port 8080 (note the space after -k)
fuser -k 8080/tcp
```

---

## Troubleshooting

**Enter WSL and go to the project first:**

```powershell
wsl
cd /mnt/e/coding/tcpserver
```

**`bind: Address already in use`**

```bash
fuser -k 8080/tcp
pkill -f "./server"
pkill -f "./client"
ss -tlnp | grep 8080    # no output = port is free
./server
```

Use `fuser -k 8080/tcp` (with a space), not `fuser -k8080/tcp`.

**`connect: Connection refused`**

Start `./server` before running `./client`.

**`Client N is not connected`**

That client already disconnected, or the number is wrong.

**`make` / `gcc: command not found`**

```bash
sudo apt update && sudo apt install build-essential
```

---

## Roadmap

| Phase | Status | Feature |
|-------|--------|---------|
| 1 | Done | Basic TCP — one message per connection |
| 2 | Done | Persistent chat with `exit` command |
| 3 | Done | Multi-client threads, command-based replies |
| 4 | Planned | Login system — username/password authentication |
| 5 | Planned | Chat rooms — shared spaces for multiple users |
| 6 | Planned | File messages — send/receive files over TCP |
| 7 | Planned | Direct messages — private one-to-one messaging |

---

## Files

```
server.c    # Multi-client server (pthread, port 8080)
client.c    # Client chat loop
Makefile    # make / make clean
```

Port and buffer size: edit `SERVER_PORT` and `BUFFER_SIZE` in both source files, then `make clean && make`.
