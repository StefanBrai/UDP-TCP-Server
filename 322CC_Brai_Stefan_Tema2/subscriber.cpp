#include <netinet/tcp.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include "utilitare.h"

int connect_to_server(const char *server_ip, int server_port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        error("ERROR invalid server IP address format");
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }

    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    return sockfd;
}

void processMessage(tcp_server_msg *data)
{
    std::cout << data->topic_name
              << " - " << data->type
              << " - " << data->data << std::endl;
}

void receiveMessages(int sockfd) {
    tcp_server_msg msg; 
    int bytesReceived = 0;

    if ((bytesReceived = recv(sockfd, &msg, sizeof(tcp_server_msg), 0)) > 0) {            
            processMessage(&msg);
    }
    else {
        std::cerr << "Failed to receive data." << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        std::cerr << "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n";
        exit(EXIT_FAILURE);
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    const char *client_id = argv[1];
    const char *server_ip = argv[2];
    int server_port = atoi(argv[3]);

    int sockfd = connect_to_server(server_ip, server_port);

    if (send(sockfd, client_id, strlen(client_id), 0) < 0)
    {
        error("send failed");
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);     
    FD_SET(sockfd, &read_fds);

    while (true)
    {
        fd_set tmp_fds = read_fds;
        if (select(sockfd + 1, &tmp_fds, NULL, NULL, NULL) == -1)
        {
            error("ERROR on select");
        }

        if (FD_ISSET(0, &tmp_fds))
        {
            char cmd_buffer[256];
            memset(cmd_buffer, 0, 256);
            std::cin.getline(cmd_buffer, 255);
            std::string command(cmd_buffer);

            if (command == "exit")
            {
                break;
            }
            else
            {
                // "subscribe <topic>" sau "unsubscribe <topic>"
                size_t first_space = command.find(' ');
                if (first_space != std::string::npos)
                {
                    std::string action = command.substr(0, first_space);
                    if (action != "subscribe" && action != "unsubscribe")
                    {
                        printf("Invalid Input :(\n");
                        continue;
                    }
                    std::string topic = command.substr(first_space + 1);
                    tcp_client_msg msg;
                    strncpy(msg.topic_name, topic.c_str(), 50);
                    msg.topic_name[50] = '\0';
                    msg.choice = (action == "subscribe" ? 1 : 0);
                    int send_rc = send(sockfd, &msg, sizeof(tcp_client_msg), 0);
                    if (send_rc <= 0)
                    {
                        error("send");
                    }
                    std::cout << (action == "subscribe" ? "Subscribed to topic " : "Unsubscribed from topic ") << topic << "\n";
                }
            }
        }

        if (FD_ISSET(sockfd, &tmp_fds))
        {
            receiveMessages(sockfd);
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
