#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int setup_multicast_socket(const char* multicast_ip, int port) {
    // 1. Create a standard UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // 2. Set the SO_REUSEADDR socket option. This is critical for multicast.
    // It allows multiple applications on the same host to bind to the same
    // port, which is necessary for listening to the same data feed.
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        return -1;
    }

    // 3. Bind the socket to the correct port on any available interface.
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any IP
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    // 4. Join the multicast group. This tells the kernel and the network
    // hardware that this socket is interested in receiving packets sent
    // to the specified multicast IP address.
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt(IP_ADD_MEMBERSHIP)");
        return -1;
    }

    printf("Successfully joined multicast group %s on port %d\n", multicast_ip, port);
    return sockfd;
}