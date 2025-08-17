#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "itch_protocol.h"
#include "byte_order.h"

#define MULTICAST_IP "233.1.1.1"
#define PORT 5000

const char* SEQ_NUM_FILE = ".sequence.dat";

uint64_t get_next_sequence_number() {
    uint64_t seq = 1;
    FILE* f = fopen(SEQ_NUM_FILE, "r");
    if (f) {
        fscanf(f, "%lu", &seq);
        fclose(f);
    }
    
    f = fopen(SEQ_NUM_FILE, "w");
    if (f) {
        fprintf(f, "%lu", seq + 1);
        fclose(f);
    }
    return seq;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buy|sell> <stock> <shares> <price>\n", argv[0]);
        return 1;
    }

    bool is_buy = (strcmp(argv[1], "buy") == 0);
    const char* stock_symbol = argv[2];
    uint32_t shares = atoi(argv[3]);
    double price_double = atof(argv[4]);
    uint32_t price_int = (uint32_t)(price_double * 10000.0);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
    dest_addr.sin_port = htons(PORT);

    AddOrderMessage msg;
    msg.message_type = 'A';
    
    srand(time(NULL) + getpid());
    msg.order_ref_num = (uint64_t)rand();

    msg.buy_sell_indicator = is_buy ? 'B' : 'S';
    msg.shares = htonl(shares);
    
    memset(msg.stock, ' ', sizeof(msg.stock));
    strncpy(msg.stock, stock_symbol, sizeof(msg.stock));
    
    msg.price = htonl(price_int);
    
    msg.sequence_number = htonll(get_next_sequence_number());

    printf("Sending Order (ID: %lu, Seq#: %lu): %s %u %s at %.4f\n",
           msg.order_ref_num, ntohll(msg.sequence_number), is_buy ? "BUY" : "SELL", shares, stock_symbol, price_double);

    sendto(sockfd, &msg, sizeof(msg), 0,
           (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    return 0;
}
