#include "utilitare.h"
#include <cmath>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

bool process_udp_message(const udp_recv_msg* udp_msg, tcp_server_msg* processed_msg) {
    if (udp_msg->type > 3) {
        std::cerr << "Invalid message type received.\n";
        return false;
    }

    strncpy(processed_msg->topic_name, udp_msg->topic_name, 50);
    processed_msg->topic_name[50] = '\0';

    long long integer;
    double real;

    switch (udp_msg->type) {
        case 0: // Integer type
            if (udp_msg->data[0] > 1) {
                std::cerr << "Invalid sign indicator for INT.\n";
                return false;
            }
            integer = ntohl(*(const uint32_t*)(udp_msg->data + 1));
            if (udp_msg->data[0] == 1) {
                integer *= -1;
            }
            sprintf(processed_msg->data, "%lld", integer);
            strcpy(processed_msg->type, "INT");
            break;

        case 1: // Short real type
            real = ntohs(*(const uint16_t*)(udp_msg->data));
            real /= 100;
            sprintf(processed_msg->data,"%.2f", real);
            strcpy(processed_msg->type, "SHORT_REAL");
            break;

        case 2: // Float type
            if (udp_msg->data[0] > 1) {
                std::cerr << "Invalid sign indicator for FLOAT.\n";
                return false;
            }
            real = ntohl(*(reinterpret_cast<const uint32_t*>(udp_msg->data + 1)));
            real /= pow(10, udp_msg->data[5]);
            if (udp_msg->data[0] == 1) {
                real *= -1;
            }
            sprintf(processed_msg->data, "%lf", real);
            strcpy(processed_msg->type, "FLOAT");
            break;

        case 3: // String type
            strcpy(processed_msg->type, "STRING");
            strncpy(processed_msg->data, udp_msg->data, sizeof(processed_msg->data) - 1);
            processed_msg->data[sizeof(processed_msg->data) - 1] = '\0';
            break;

        default:
            std::cerr << "Unknown data type received.\n";
            return false;
    }

    return true;
}
