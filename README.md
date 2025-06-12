# Chatroom Project

A simple cross-platform chatroom application in C supporting multiple clients and private messaging.

## Features

- Multi-client chatroom
- Private messaging with `/msg` command
- Cross-platform: Windows and Unix/Linux

## Requirements

- GCC (MinGW recommended for Windows)
- Winsock2 (Windows only)
- pthreads (Unix/Linux only)

## Build Instructions

**On Windows (using MinGW):**
```bash
gcc server.c -o server.exe -lws2_32
gcc client.c -o client.exe -lws2_32
```

**On Unix/Linux:**
```bash
gcc server.c -o server -lpthread
gcc client.c -o client -lpthread
```

## Run Instructions

**Start the server in one terminal:**
```bash
./server.exe   # On Windows
./server       # On Unix/Linux
```

**Start the client in a separate terminal:**
```bash
./client.exe 127.0.0.1 8080   # On Windows
./client 127.0.0.1 8080       # On Unix/Linux
```

> **Note:** The server and each client must be run in separate terminal windows. You can open multiple terminals to connect multiple clients to the chatroom.

## Usage
- Enter your name when prompted by the client.
- Type messages to chat with everyone.
- Use `/msg <username> <message>` to send a private message.
- Type `/exit` to leave the chatroom.

## .gitignore
Add the following to your `.gitignore`:
```
server.exe
client.exe
server
client
```

---
