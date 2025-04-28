#include <cstdint>
#include <list>
#include <string>
#include <regex>
#define MAX_BUFFER 256
#define EMPTY_ID_SOCKET_LINK 9999

struct tcp_server_msg {
    char ip[16];
    uint16_t udp_port;
    char topic_name[51];
    char type[11];
    char data[1501];
}__attribute__((packed));

struct udp_recv_msg
{
    char topic_name[50];
    uint8_t type;
    char data[1501];
} __attribute__((packed));

struct tcp_client_msg
{
    char topic_name[51];
    uint8_t choice;
} __attribute__((packed));

struct client_data_base {
    char id[11];
    int socket;
    std::list<std::string> subbed_topics; 
};

void error(const char *msg);

bool process_udp_message(const udp_recv_msg* udp_msg, tcp_server_msg* processed_msg);