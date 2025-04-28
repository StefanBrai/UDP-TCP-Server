#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <list>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <vector>
#include "utilitare.h"

std::list<client_data_base> clients_list;
std ::list<std::string> topic_map;

std::list<std::string> process_topic_with_wildcard(const std::string &wildcard_topic)
{
    std::list<std::string> matched_topics;
    std::string regex_pattern = wildcard_topic;

    if (wildcard_topic.find('*') == std::string::npos && wildcard_topic.find('+') == std::string::npos)
    {
        // Fara wildcarduri in el
        auto it = std::find(topic_map.begin(), topic_map.end(), wildcard_topic);
        if (it != topic_map.end())
        {
            matched_topics.push_back(wildcard_topic);
        }
        return matched_topics;
    }

    std::regex escape_regex("([.^$|()\\[\\]{}?])");
    regex_pattern = std::regex_replace(regex_pattern, escape_regex, "\\$1");

    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\+"), "([^/]+)");

    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\*"), "(.*)");

    std::regex final_regex("^" + regex_pattern + "$");

    for (const auto &topic : topic_map)
    {
        if (std::regex_match(topic, final_regex))
        {
            matched_topics.push_back(topic);
        }
    }

    return matched_topics;
}

void subscribe(client_data_base *client, std::string topic_param)
{
    std ::list<std::string> list_of_topics = process_topic_with_wildcard(topic_param);
    for (auto &topic : list_of_topics)
    {
        auto it = std::find(topic_map.begin(), topic_map.end(), topic);

        if (it == topic_map.end())
        {
            topic_map.push_back(topic);
        }

        auto sub_it = std::find(client->subbed_topics.begin(), client->subbed_topics.end(), topic);
        if (sub_it == client->subbed_topics.end())
        {
            client->subbed_topics.push_back(topic);
        }
    }
}

void unsubscribe(client_data_base *client, std::string topic_param)
{
    std ::list<std::string> list_of_topics = process_topic_with_wildcard(topic_param);
    for (auto &topic : list_of_topics)
    {
        auto it = std::find(client->subbed_topics.begin(), client->subbed_topics.end(), topic);

        if (it != client->subbed_topics.end())
        {
            client->subbed_topics.erase(it);
        }
    }
}

void send_to_subscribers(const std::string topic, tcp_server_msg *data)
{
    for (auto &client : clients_list)
    {
        auto it = std::find(client.subbed_topics.begin(), client.subbed_topics.end(), topic);
        if (it != client.subbed_topics.end() && client.socket != EMPTY_ID_SOCKET_LINK)
        {
            if (send(client.socket, data, sizeof(tcp_server_msg), 0) < 0)
            {
                std::cerr << "Failed to send data to client ID " << client.id << " on socket " << client.socket << std::endl;
                error("send");
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int tcp_sockfd, udp_sockfd, portno;
    char buffer[MAX_BUFFER];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int n, reuse = 1, flag = 1;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tcp_sockfd < 0 || udp_sockfd < 0)
        error("ERROR opening socket");

    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
        error("setsockopt(TCP_NODELAY) failed");

    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(tcp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ||
        bind(udp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(tcp_sockfd, INT16_MAX);
    clilen = sizeof(cli_addr);

    fd_set active_fds;
    FD_ZERO(&active_fds);
    FD_SET(0, &active_fds); // 0 este stdin
    FD_SET(tcp_sockfd, &active_fds);
    FD_SET(udp_sockfd, &active_fds);
    int max_fd = std::max(tcp_sockfd, udp_sockfd);

    while (true)
    {
        fd_set aux_fds = active_fds;
        memset(buffer, 0, MAX_BUFFER);

        if (select(max_fd + 1, &aux_fds, NULL, NULL, NULL) == -1)
            error("ERROR on select");

        for (int i = 0; i <= max_fd; ++i)
        {
            if (FD_ISSET(i, &aux_fds))
            {
                if (i == 0)
                { // stdin
                    fgets(buffer, MAX_BUFFER, stdin);
                    if (strcmp(buffer, "exit\n") == 0)
                    {
                        printf("Shutting down server.\n");
                        close(tcp_sockfd);
                        close(udp_sockfd);
                        exit(0);
                    }
                    else
                    {
                        printf("`exit` is the only command available.\n");
                    }
                }
                else if (i == tcp_sockfd)
                {
                    int newclient_fd = accept(tcp_sockfd, (struct sockaddr *)&cli_addr, &clilen);
                    if (newclient_fd < 0)
                        error("ERROR on accept");

                    setsockopt(newclient_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    char client_id_buffer[256];
                    memset(client_id_buffer, 0, sizeof(client_id_buffer));

                    if (recv(newclient_fd, client_id_buffer, sizeof(client_id_buffer), 0) <= 0)
                    {
                        printf("Failed to read client ID.\n");
                        close(newclient_fd);
                    }
                    else
                    {
                        int is_client_connected_or_doesnt_exist = 0; // 0 - doesn t exist
                                                                     // 1 - connected
                        for (auto &client : clients_list)
                        {
                            if (strcmp(client.id, client_id_buffer) == 0)
                            {
                                if (client.socket != EMPTY_ID_SOCKET_LINK)
                                {
                                    close(newclient_fd);
                                    std::cout << "Client " << client_id_buffer << " already connected." << std::endl;
                                    is_client_connected_or_doesnt_exist = 1;
                                    break;
                                }
                                else
                                {
                                    client.socket = newclient_fd;
                                    is_client_connected_or_doesnt_exist = 1;
                                    FD_SET(newclient_fd, &active_fds);
                                    if (newclient_fd > max_fd)
                                    {
                                        max_fd = newclient_fd;
                                    }
                                    printf("New client %s connected from %s:%hu.\n", client_id_buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                                    break;
                                }
                            }
                        }
                        if (!is_client_connected_or_doesnt_exist)
                        {
                            client_data_base new_client;
                            strcpy(new_client.id, client_id_buffer);
                            new_client.socket = newclient_fd;
                            clients_list.push_back(new_client);
                            printf("New client %s connected from %s:%hu.\n", client_id_buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                            FD_SET(newclient_fd, &active_fds);
                            if (newclient_fd > max_fd)
                            {
                                max_fd = newclient_fd;
                            }
                        }
                    }
                }
                else if (i == udp_sockfd)
                {
                    udp_recv_msg msg;
                    memset(&msg, 0, sizeof(udp_recv_msg));
                    n = recvfrom(udp_sockfd, &msg, sizeof(udp_recv_msg), 0, (struct sockaddr *)&cli_addr, &clilen);
                    if (n < 0)
                        error("ERROR in recvfrom");

                    tcp_server_msg processed_msg;
                    processed_msg.udp_port = ntohs(cli_addr.sin_port);
                    strcpy(processed_msg.ip, inet_ntoa(cli_addr.sin_addr));

                    if (process_udp_message(&msg, &processed_msg))
                    {
                        auto it = std::find(topic_map.begin(), topic_map.end(), msg.topic_name);

                        if (it == topic_map.end())
                        {
                            topic_map.push_back(msg.topic_name);
                        }
                        else
                        {
                            send_to_subscribers(msg.topic_name, &processed_msg);
                        }
                    }
                }
                else
                {
                    tcp_client_msg msg;
                    n = recv(i, &msg, sizeof(tcp_client_msg), 0);
                    auto it = std::find_if(clients_list.begin(), clients_list.end(), [i](const client_data_base &client)
                                           { return client.socket == i; });

                    if (n < 0)
                    {
                        error("recv");
                    }

                    if (n == 0)
                    { // Client disconnected
                        it->socket = EMPTY_ID_SOCKET_LINK;
                        printf("Client %s disconnected.\n", it->id);
                        close(i);
                        FD_CLR(i, &active_fds);
                        for (int k = max_fd; k > 2; k--)
                        {
                            if (FD_ISSET(k, &active_fds))
                            {
                                max_fd = k;
                                break;
                            }
                        }
                    }
                    else
                    {

                        if (msg.choice == 1)
                        {
                            subscribe(&(*it), std::string(msg.topic_name));
                        }
                        else if (msg.choice == 0)
                        {
                            unsubscribe(&(*it), std::string(msg.topic_name));
                        }
                    }
                }
            }
        }
    }

    for (int i = 2; i <= max_fd; ++i)
    {
        if (FD_ISSET(i, &active_fds))
        {
            close(i);
        }
    }

    return 0;
}
