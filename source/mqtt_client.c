#include <network.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MQTT_BROKER_IP "5.196.78.28" // test.mosquitto.org
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 60
#define MQTT_CLIENT_ID "wii_client_12345"
#define MQTT_TOPIC "wii/test"

static int mqtt_send_connect(int sock) {
    unsigned char packet[128];
    int i = 0;
    packet[i++] = 0x10; // CONNECT
    int rem_len_pos = i++;
    packet[i++] = 0x00; packet[i++] = 0x04;
    packet[i++] = 'M'; packet[i++] = 'Q'; packet[i++] = 'T'; packet[i++] = 'T';
    packet[i++] = 0x04;
    packet[i++] = 0x02;
    packet[i++] = 0x00; packet[i++] = MQTT_KEEPALIVE;
    int client_id_len = strlen(MQTT_CLIENT_ID);
    packet[i++] = 0x00; packet[i++] = client_id_len;
    memcpy(&packet[i], MQTT_CLIENT_ID, client_id_len);
    i += client_id_len;
    packet[rem_len_pos] = i - 2;
    return net_send(sock, (char*)packet, i, 0);
}

static int mqtt_send_subscribe(int sock, const char *topic) {
    unsigned char packet[128];
    int i = 0;
    static unsigned short packet_id = 1;
    packet[i++] = 0x82;
    int rem_len_pos = i++;
    packet[i++] = (packet_id >> 8) & 0xFF;
    packet[i++] = packet_id & 0xFF;
    int topic_len = strlen(topic);
    packet[i++] = (topic_len >> 8) & 0xFF;
    packet[i++] = topic_len & 0xFF;
    memcpy(&packet[i], topic, topic_len);
    i += topic_len;
    packet[i++] = 0x00;
    packet[rem_len_pos] = i - 2;
    return net_send(sock, (char*)packet, i, 0);
}


void mqtt_receive_loop() {
    extern volatile bool running;
    int first_connect = 1;
    while (running) {
        int sock;
        struct sockaddr_in broker;
        char buffer[512];
        int bytes = -1;

        if (first_connect)
            printf("Connecting to MQTT broker...\n");
        sock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Socket creation failed\n");
            usleep(2000000); // Wait 2s before retry
            continue;
        }
        memset(&broker, 0, sizeof(broker));
        broker.sin_family = AF_INET;
        broker.sin_port = htons(MQTT_PORT);
        broker.sin_addr.s_addr = inet_addr(MQTT_BROKER_IP);

        if (net_connect(sock, (struct sockaddr*)&broker, sizeof(broker)) < 0) {
            printf("Connection to broker failed\n");
            net_close(sock);
            usleep(2000000); // Wait 2s before retry
            continue;
        }
        if (first_connect)
            printf("Connected to broker\n");

        // Set socket to non-blocking mode (libogc: use net_ioctl with FIONBIO)
        u32 nbio = 1;
        net_ioctl(sock, FIONBIO, &nbio);

        if (mqtt_send_connect(sock) < 0) {
            printf("Failed to send CONNECT\n");
            net_close(sock);
            usleep(2000000);
            continue;
        }

        if (first_connect)
            printf("CONNECT sent\n");

        // Wait for CONNACK (with running check)
        int wait_count = 0;
        bytes = -1;
        while (running) {
            bytes = net_recv(sock, buffer, sizeof(buffer), 0);
            if (bytes < 0) {
                usleep(10000); // 10ms
                if (++wait_count > 500) { // 5s timeout
                    printf("Timeout waiting for CONNACK\n");
                    net_close(sock);
                    break;
                }
                continue;
            }
            if (bytes < 4 || (unsigned char)buffer[0] != 0x20) {
                printf("No CONNACK\n");
                net_close(sock);
                break;
            }
            break;
        }
        if (!running) { net_close(sock); break; }
        if (bytes < 4 || (unsigned char)buffer[0] != 0x20) {
            usleep(2000000);
            continue;
        }
        if (first_connect)
            printf("CONNACK received\n");

        if (mqtt_send_subscribe(sock, MQTT_TOPIC) < 0) {
            if (first_connect)
                printf("Failed to send SUBSCRIBE\n");
            net_close(sock);
            usleep(2000000);
            continue;
        }
        if (first_connect)
            printf("SUBSCRIBE sent for topic: %s\n", MQTT_TOPIC);

        // Wait for SUBACK (with running check)
        wait_count = 0;
        bytes = -1;
        while (running) {
            bytes = net_recv(sock, buffer, sizeof(buffer), 0);
            if (bytes < 0) {
                usleep(10000);
                if (++wait_count > 500) {
                    printf("Timeout waiting for SUBACK\n");
                    net_close(sock);
                    break;
                }
                continue;
            }
            if (bytes < 5 || (unsigned char)buffer[0] != 0x90) {
                printf("No SUBACK\n");
                net_close(sock);
                break;
            }
            break;
        }
        if (!running) { net_close(sock); break; }
        if (bytes < 5 || (unsigned char)buffer[0] != 0x90) {
            usleep(2000000);
            continue;
        }
        if (first_connect)
            printf("SUBACK received\n");

        // Main receive loop, check running flag
        while (running) {
            bytes = net_recv(sock, buffer, sizeof(buffer), 0);
            if (bytes > 0) {
                if ((unsigned char)buffer[0] == 0x30) {
                    int topic_len = (buffer[2] << 8) | buffer[3];
                    int payload_offset = 4 + topic_len;
                    int payload_len = bytes - payload_offset;
                    printf("Message: %.*s\n", payload_len, &buffer[payload_offset]);
                }
            } else if (bytes == 0) {
                if (first_connect)
                    printf("Broker closed connection\n");
                break;
            } else {
                usleep(10000); // 10ms
            }
        }
        net_close(sock);
        if (first_connect)
            first_connect = 0;
        if (running) {
            if (first_connect)
                printf("Reconnecting to broker in 2 seconds...\n");
            usleep(2000000); // Wait 2s before reconnect
        }
    }
}
