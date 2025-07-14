#include <network.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 5000
#define SERVER_IP "192.168.2.2" // Change this to your server's IP if needed

int tcp_connect_and_listen(const char *server_ip, int port) {
    int sock;
    struct sockaddr_in server;
    char buffer[256];
    int bytes_received;

    printf("tcp_connect_and_listen: starting\n");
    // Initialize network subsystem
    if (net_init() < 0) {
        printf("Network init failed\n");
        return -1;
    }
    printf("Network initialized\n");

    // Create socket
    printf("Creating socket...\n");
    sock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Socket creation failed\n");
        return -1;
    }
    printf("Socket created\n");

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    printf("Connecting to server...\n");
    if (net_connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        net_close(sock);
        return -1;
    }
    printf("Connected to server %s:%d\n", server_ip, port);

    // Listen for data in a loop until server closes connection or error
    while (1) {
        bytes_received = net_recv(sock, buffer, sizeof(buffer)-1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received: %s", buffer);
        } else if (bytes_received == 0) {
            printf("Server closed connection\n");
            break;
        } else {
            printf("No data received or error\n");
            break;
        }
    }
    net_close(sock);
    return 0;
}
