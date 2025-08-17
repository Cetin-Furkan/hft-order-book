#ifndef NETWORK_H
#define NETWORK_H

/**
 * @brief Sets up a UDP socket to listen to a specific multicast group.
 *
 * @param multicast_ip The IP address of the multicast group to join.
 * @param port The port to listen on.
 * @return The socket file descriptor on success, or -1 on error.
 */
int setup_multicast_socket(const char* multicast_ip, int port);

#endif // NETWORK_H