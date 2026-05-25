# HTTP Server From Scratch

## Overview

This project is a simple HTTP Server and Client written in C without using backend frameworks.  
The main purpose of the project is to understand backend systems from the lower layers before learning higher-level abstractions such as ExpressJS, Spring Boot, or Django.

The project focuses on:

- TCP Socket Programming
- HTTP Protocol
- Non-blocking I/O
- Event Loop Architecture
- kqueue I/O Multiplexing
- Stateful HTTP Parsing

---

# Motivation

Most developers learn backend through frameworks first, but often do not fully understand:

- How HTTP requests are processed internally
- How servers handle multiple connections
- Why event-driven systems scale efficiently
- How event loops optimize backend performance

This project was built to understand the foundation behind modern backend runtimes such as:

- :contentReference[oaicite:0]{index=0}
- :contentReference[oaicite:1]{index=1}
- :contentReference[oaicite:2]{index=2}

---

# Core Concepts

## Event Loop

Instead of creating one thread per connection, the server uses a single event loop to manage multiple clients asynchronously.

```text
while(true)
{
    wait_for_events();
    handle_ready_connections();
}
```

This helps reduce:

- Thread overhead
- Blocking operations
- Context switching cost

---

## Non-blocking I/O

The server uses non-blocking sockets:

```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

This allows the server to continue processing other connections without waiting for one client.

---

## HTTP State Machine

HTTP requests are parsed using a state machine:

```text
REQUEST_LINE
    ↓
HEADERS
    ↓
BODY
    ↓
DONE
```

This helps the server process partial network data correctly.

---

# Program Flow

```text
Client
   ↓
TCP Socket
   ↓
Event Loop (kqueue)
   ↓
HTTP Parser
   ↓
Request Handler
   ↓
HTTP Response
```

---

# Features

## Server

- Non-blocking socket server
- Event-driven architecture
- HTTP request parser
- GET / POST / DELETE support
- kqueue-based event handling

## Client

- Manual HTTP request builder
- HTTP response parser
- Interactive request sending

---

# Build

## Compile Server

```bash
gcc server.c -o server
```

## Compile Client

```bash
gcc client.c -o client
```

---

# Run

## Start Server

```bash
./server
```

## Start Client

```bash
./client
```

---

# Educational Goal

The goal of this project is not to build a production framework.

The goal is to understand the backend foundation underneath modern frameworks, especially:

- Event loops
- Async networking
- HTTP internals
- Backend runtime architecture
- System-level concurrency

---

# Source Files

- Server: :contentReference[oaicite:3]{index=3}
- Client: :contentReference[oaicite:4]{index=4}
