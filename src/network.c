#define _GNU_SOURCE

#include "network.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

// --- Static Helper Function Prototypes ---
[[nodiscard]] static int network_init_internal(NetworkThreadState* state, uint16_t port);
static void network_shutdown_internal(NetworkThreadState* state);
[[nodiscard]] static int make_socket_non_blocking(int sfd);
static void handle_new_connection(NetworkThreadState* state);
static void handle_client_data(NetworkThreadState* state, ConnectionState* conn);
static void close_connection(NetworkThreadState* state, ConnectionState* conn);

// --- Main Thread Function ---

void* network_thread_main(void* arg) {
    NetworkThreadState* state = (NetworkThreadState*)arg;
    uint16_t port = 8080;

    if (network_init_internal(state, port) != 0) {
        fprintf(stderr, "Network thread failed to initialize.\n");
        return nullptr;
    }

    printf("Network thread started. Listening on port %d.\n", port);
    struct epoll_event events[MAX_EPOLL_EVENTS];

    while (atomic_load(state->running)) {
        int n = epoll_wait(state->epoll_fd, events, MAX_EPOLL_EVENTS, 1000);
        for (int i = 0; i < n; i++) {
            void* ptr = events[i].data.ptr;
            uint32_t ev = events[i].events;

            if (ptr == state) { // Event is on the listening socket
                if ((ev & EPOLLERR) || (ev & EPOLLHUP)) {
                    fprintf(stderr, "Fatal error on listening socket. Shutting down.\n");
                    atomic_store(state->running, false);
                    continue;
                }
                handle_new_connection(state);
            } else { // Event is on a client socket
                ConnectionState* conn = (ConnectionState*)ptr;
                if ((ev & EPOLLERR) || (ev & EPOLLHUP)) {
                    fprintf(stderr, "epoll error on fd %d, closing connection.\n", conn->fd);
                    close_connection(state, conn);
                    continue;
                }
                handle_client_data(state, conn);
            }
        }
    }

    network_shutdown_internal(state);
    printf("Network thread shutting down.\n");
    return nullptr;
}

// --- Internal Helper Functions ---

[[nodiscard]] static int network_init_internal(NetworkThreadState* state, uint16_t port) {
    state->epoll_fd = epoll_create1(0);
    if (state->epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    state->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->listen_fd == -1) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(state->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(SO_REUSEADDR)");
        close(state->listen_fd);
        return -1;
    }
    if (setsockopt(state->listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt(SO_REUSEPORT)");
        close(state->listen_fd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(state->listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(state->listen_fd);
        return -1;
    }

    if (make_socket_non_blocking(state->listen_fd) == -1) {
        close(state->listen_fd);
        return -1;
    }

    if (listen(state->listen_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(state->listen_fd);
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = state;
    if (epoll_ctl(state->epoll_fd, EPOLL_CTL_ADD, state->listen_fd, &event) == -1) {
        perror("epoll_ctl: listen_fd");
        close(state->listen_fd);
        return -1;
    }
    return 0;
}

static void network_shutdown_internal(NetworkThreadState* state) {
    close(state->listen_fd);
    close(state->epoll_fd);
}

[[nodiscard]] static int make_socket_non_blocking(int sfd) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sfd, F_SETFL, flags) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

static void close_connection(NetworkThreadState* state, ConnectionState* conn) {
    if (epoll_ctl(state->epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL");
    }
    close(conn->fd);
    free(conn);
}

static void handle_new_connection(NetworkThreadState* state) {
    while (1) {
        int conn_sock = accept(state->listen_fd, nullptr, nullptr);
        if (conn_sock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else { perror("accept"); break; }
        }

        if (make_socket_non_blocking(conn_sock) != 0) {
            close(conn_sock);
            continue;
        }

        ConnectionState* conn_state = malloc(sizeof(ConnectionState));
        if (conn_state == nullptr) {
            fprintf(stderr, "Failed to allocate memory for new connection state.\n");
            close(conn_sock);
            continue;
        }
        conn_state->fd = conn_sock;
        conn_state->buffer_used = 0;

        struct epoll_event event = { .events = EPOLLIN | EPOLLET };
        event.data.ptr = conn_state;
        if (epoll_ctl(state->epoll_fd, EPOLL_CTL_ADD, conn_sock, &event) == -1) {
            perror("epoll_ctl: conn_sock");
            free(conn_state);
            close(conn_sock);
        }
        printf("Accepted new connection on fd %d\n", conn_sock);
    }
}

// FIX: This function now correctly implements the EPOLLET contract.
static void handle_client_data(NetworkThreadState* state, ConnectionState* conn) {
    bool connection_closed = false;

    // We must loop and read from the socket until it returns EAGAIN.
    while (1) {
        ssize_t bytes_read = read(conn->fd, conn->buffer + conn->buffer_used, sizeof(conn->buffer) - conn->buffer_used);

        if (bytes_read == -1) {
            // If errno is EAGAIN, we have read all the data. Stop looping.
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // A real error occurred.
            perror("read");
            connection_closed = true;
            break;
        } else if (bytes_read == 0) {
            // The client has closed the connection.
            printf("Client on fd %d closed connection.\n", conn->fd);
            connection_closed = true;
            break;
        }

        conn->buffer_used += bytes_read;
    }

    // Now, process all complete messages that are in our buffer.
    while (conn->buffer_used > 0) {
        if (conn->buffer_used < sizeof(uint8_t)) break;

        uint8_t msg_type = conn->buffer[0];
        size_t msg_size = 0;

        if (msg_type == 'N') msg_size = sizeof(NewOrderMessage);
        else if (msg_type == 'C') msg_size = sizeof(CancelOrderMessage);
        else if (msg_type == 'X') msg_size = sizeof(ShutdownMessage);
        else {
            fprintf(stderr, "Protocol error on fd %d: unknown message type '%c'. Closing connection.\n", conn->fd, msg_type);
            connection_closed = true;
            break;
        }

        if (conn->buffer_used >= msg_size) {
            ProtocolMessage p_msg;
            memset(&p_msg, 0, sizeof(p_msg));
            memcpy(&p_msg, conn->buffer, msg_size);

            if (!spsc_ring_buffer_push(state->ring_buffer, &p_msg)) {
                fprintf(stderr, "Warning: Ring buffer full. Discarding message.\n");
            }

            memmove(conn->buffer, conn->buffer + msg_size, conn->buffer_used - msg_size);
            conn->buffer_used -= msg_size;
        } else {
            break; // Incomplete message in buffer.
        }
    }

    if (connection_closed) {
        close_connection(state, conn);
    }
}
