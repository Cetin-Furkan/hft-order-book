/*
 * NASDAQ ITCH 5.0 REPLAY SERVER (Production Hardened)
 * Platform: Arch Linux (glibc)
 * Standard: C23 (-std=c2x)
 */

#define _GNU_SOURCE // Required for modern linux socket options

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

// Linux Native Networking & Endianness
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <endian.h>   // <--- The "Real" Linux Endian header (htobe64)

// C23 Safe Math
#include <stdckdint.h>

// Your saved protocol header
#include "itch_protocol.h"

// -----------------------------------------------------------------------------
// CONFIGURATION
// -----------------------------------------------------------------------------
#define MCAST_GROUP     "239.0.0.1"
#define MCAST_PORT      5000
#define PACKET_MTU      1400        // Optimal for avoiding IP fragmentation
#define HEADER_SIZE     20          // MoldUDP64 Header Size
#define MAX_MSG_SIZE    64          // Max ITCH message size
#define SESSION_NAME    "NASDAQ_DAY"
#define BUSY_WAIT_NS    2000        // Tuned for Wi-Fi (2us delay)

// -----------------------------------------------------------------------------
// MOLDUDP64 HEADER
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
typedef struct {
    char     session[10];   // Session Name
    uint64_t sequence;      // First Msg Seq Num (Big Endian)
    uint16_t count;         // Msg Count (Big Endian)
} mold_header_t;
#pragma pack(pop)

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------
void nanospin(long ns) {
    if (ns <= 0) return;
    struct timespec req = {0, ns};
    nanosleep(&req, NULL);
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <itch_file.bin> [interface_ip]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filepath = argv[1];
    const char *iface_ip = (argc > 2) ? argv[2] : "0.0.0.0";

    // 1. Open File (Implicit Large File Support on 64-bit Linux)
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("FATAL: Cannot open source file");
        return EXIT_FAILURE;
    }

    // 2. Setup Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("FATAL: Socket creation failed");
        fclose(fp);
        return EXIT_FAILURE;
    }

    // 3. Socket Options (The Production Stuff)
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("WARN: Failed to set SO_REUSEADDR");
    }

    // Bind outgoing interface
    struct in_addr local_interface;
    local_interface.s_addr = inet_addr(iface_ip);
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, 
                   (char *)&local_interface, sizeof(local_interface)) < 0) {
        perror("WARN: Failed to set Outgoing Interface (using default route)");
    }

    // Set TTL (Crucial for physical switches/routers)
    // Default is 1. We set to 16 to allow passing through local switches.
    uint8_t ttl = 16;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("WARN: Failed to set TTL");
    }

    // Enable Loopback (For local testing)
    uint8_t loop = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    // Destination setup
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(MCAST_GROUP);
    dest_addr.sin_port = htons(MCAST_PORT);

    // 4. State Variables
    uint8_t  packet_buf[PACKET_MTU];
    uint8_t  msg_buf[MAX_MSG_SIZE];
    uint64_t global_seq_num = 1;
    uint16_t msgs_in_packet = 0;
    size_t   packet_offset  = HEADER_SIZE;

    // Initialize Header Session Name
    mold_header_t *hdr = (mold_header_t *)packet_buf;
    memset(hdr->session, ' ', 10);
    memcpy(hdr->session, SESSION_NAME, strlen(SESSION_NAME) < 10 ? strlen(SESSION_NAME) : 10);

    printf("--> SYSTEM READY. Streaming %s to %s:%d\n", filepath, MCAST_GROUP, MCAST_PORT);

    // 5. Hot Loop
    while (true) {
        // A. Read Length (2 Bytes Big Endian from file)
        uint16_t msg_len_be;
        if (fread(&msg_len_be, 2, 1, fp) != 1) break; 

        // Convert to Host order to check logic
        uint16_t msg_len = be16toh(msg_len_be); // <--- Native Linux Macro

        if (msg_len == 0) continue; // Skip empty (rare but possible padding)

        // B. Read Payload
        if (fread(msg_buf, 1, msg_len, fp) != msg_len) break;

        // C. Check Packet Fit
        if (packet_offset + 2 + msg_len > PACKET_MTU) {
            // Write Header
            hdr->sequence = htobe64(global_seq_num); // <--- Native Linux Macro
            hdr->count    = htobe16(msgs_in_packet); // <--- Native Linux Macro

            // Send
            sendto(sock, packet_buf, packet_offset, 0, 
                   (struct sockaddr*)&dest_addr, sizeof(dest_addr));

            // Safe Increment
            if (ckd_add(&global_seq_num, global_seq_num, msgs_in_packet)) {
                fprintf(stderr, "FATAL: Sequence Overflow\n");
                break;
            }

            // Reset
            packet_offset = HEADER_SIZE;
            msgs_in_packet = 0;
            
            // Rate Limit
            nanospin(BUSY_WAIT_NS);
        }

        // D. Append (Length + Body)
        memcpy(packet_buf + packet_offset, &msg_len_be, 2);
        packet_offset += 2;
        memcpy(packet_buf + packet_offset, msg_buf, msg_len);
        packet_offset += msg_len;
        msgs_in_packet++;
    }

    // 6. Flush Final Packet (CRITICAL: Don't lose the last trades)
    if (msgs_in_packet > 0) {
        hdr->sequence = htobe64(global_seq_num);
        hdr->count    = htobe16(msgs_in_packet);
        sendto(sock, packet_buf, packet_offset, 0, 
               (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    }

    printf("--> REPLAY COMPLETE. Final Seq: %lu\n", global_seq_num);
    
    close(sock);
    fclose(fp);
    return EXIT_SUCCESS;
}
