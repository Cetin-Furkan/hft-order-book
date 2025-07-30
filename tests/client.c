#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/protocol.h" // Include our protocol definition

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Helper function to send a new order message
void send_new_order(int sockfd, uint64_t id, uint8_t side, uint64_t price, uint32_t quantity) {
    NewOrderMessage msg;
    msg.msg_type = 'N';
    msg.side = side;
    msg.order_id = id;
    msg.price = price;
    msg.quantity = quantity;

    printf("Sending New Order: ID=%lu, Side=%c, Price=%lu, Qty=%u\n", id, side, price, quantity);
    if (send(sockfd, &msg, sizeof(msg), 0) < 0) {
        perror("send");
    }
}

// Helper function to send a cancel order message
void send_cancel_order(int sockfd, uint64_t id) {
    CancelOrderMessage msg;
    msg.msg_type = 'C';
    msg.order_id = id;

    printf("Sending Cancel Order: ID=%lu\n", id);
    if (send(sockfd, &msg, sizeof(msg), 0) < 0) {
        perror("send");
    }
}

// Helper function to send the shutdown message
void send_shutdown(int sockfd) {
    ShutdownMessage msg;
    msg.msg_type = 'X';
    printf("Sending Shutdown Signal...\n");
    if (send(sockfd, &msg, sizeof(msg), 0) < 0) {
        perror("send");
    }
}

int main(int argc, char* argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Connected to server.\n");

    if (argc > 1 && strcmp(argv[1], "shutdown") == 0) {
        send_shutdown(sockfd);
    } else {
        printf("--- Sending test order sequence ---\n");
        send_new_order(sockfd, 1, 'B', 9900, 20);
        sleep(1);
        send_new_order(sockfd, 2, 'S', 10100, 15);
        sleep(1);
        send_new_order(sockfd, 3, 'B', 10000, 5);
        sleep(1);
        
        printf("\n--- Sending cancel for order 1 ---\n");
        send_cancel_order(sockfd, 1);
        
        // FIX: Removed the automatic shutdown call from the default test sequence.
        // The server will now remain running after the client disconnects.
    }

    printf("Finished sending commands. Closing connection.\n");
    close(sockfd);

    return EXIT_SUCCESS;
}
