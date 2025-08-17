#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "itch_protocol.h" // We reuse the same protocol header

#define MULTICAST_IP "233.1.1.1"
#define PORT 5000

// --- Global sequence number ---
// We'll use a simple global variable to keep track of the sequence.
static uint64_t sequence_counter = 1;

// --- Function Prototypes ---
void send_add_order(int sockfd, struct sockaddr_in* dest_addr);
void send_order_executed(int sockfd, struct sockaddr_in* dest_addr);
void send_order_cancel(int sockfd, struct sockaddr_in* dest_addr);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <add|exec|cancel> [out_of_order]\n", argv[0]);
        return 1;
    }

    // 1. Create a standard UDP socket for sending.
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // 2. Define the destination address (our multicast group).
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
    dest_addr.sin_port = htons(PORT);

    // --- Special handling for out-of-order test ---
    // If a second argument is provided, we'll send a future packet first.
    if (argc == 3 && strcmp(argv[2], "out_of_order") == 0) {
        printf("--- Sending a future packet first for testing ---\n");
        // Temporarily jump the sequence number ahead
        sequence_counter = 10;
        send_add_order(sockfd, &dest_addr);
        // Reset sequence for the main packet
        sequence_counter = 1;
        printf("---------------------------------------------\n");
    }

    // 3. Check the command-line argument and call the appropriate function.
    if (strcmp(argv[1], "add") == 0) {
        send_add_order(sockfd, &dest_addr);
    } else if (strcmp(argv[1], "exec") == 0) {
        send_order_executed(sockfd, &dest_addr);
    } else if (strcmp(argv[1], "cancel") == 0) {
        send_order_cancel(sockfd, &dest_addr);
    } else {
        fprintf(stderr, "Invalid message type: %s\n", argv[1]);
        return 1;
    }

    return 0;
}

void send_add_order(int sockfd, struct sockaddr_in* dest_addr) {
    AddOrderMessage msg;
    msg.sequence_number = sequence_counter++;
    msg.message_type = 'A';
    msg.stock_locate = htons(123);
    msg.tracking_number = htons(456);
    msg.timestamp = 0;
    msg.order_ref_num = 789;
    msg.buy_sell_indicator = 'B';
    msg.shares = htonl(100);
    strncpy(msg.stock, "INTC    ", 8);
    msg.price = htonl(505000);

    printf("Sending 'Add Order' (Seq#: %lu)...\n", msg.sequence_number);
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)dest_addr, sizeof(*dest_addr));
}

void send_order_executed(int sockfd, struct sockaddr_in* dest_addr) {
    OrderExecutedMessage msg;
    msg.sequence_number = sequence_counter++;
    msg.message_type = 'E';
    msg.stock_locate = htons(123);
    msg.tracking_number = htons(457);
    msg.timestamp = 1000;
    msg.order_ref_num = 789;
    msg.executed_shares = htonl(50);
    msg.match_number = 999;

    printf("Sending 'Order Executed' (Seq#: %lu)...\n", msg.sequence_number);
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)dest_addr, sizeof(*dest_addr));
}

void send_order_cancel(int sockfd, struct sockaddr_in* dest_addr) {
    OrderCancelMessage msg;
    msg.sequence_number = sequence_counter++;
    msg.message_type = 'X';
    msg.stock_locate = htons(123);
    msg.tracking_number = htons(458);
    msg.timestamp = 2000;
    msg.order_ref_num = 789;
    msg.canceled_shares = htonl(50);

    printf("Sending 'Order Cancel' (Seq#: %lu)...\n", msg.sequence_number);
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)dest_addr, sizeof(*dest_addr));
}
