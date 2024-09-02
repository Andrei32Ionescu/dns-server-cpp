#pragma once

struct Resolver_Info {
    std::string ip;
    std::string port;
    int socket;
    sockaddr_in addr;
};

std::pair<int, int> create_server(int port_number);
void setup_resolver_socket(Resolver_Info& resolver);