#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "server_init.hpp"

std::pair<int, int> create_server(int port_number) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "..." << std::endl;
        return {1, udpSocket};
    }

    // Ensures that 'Address already in use' errors are not encountered during testing
    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "SO_REUSEPORT failed: " << strerror(errno) << std::endl;
        return {1, udpSocket};
    }

    sockaddr_in serv_addr = { .sin_family = AF_INET,
                                .sin_port = htons(port_number),
                                .sin_addr = { htonl(INADDR_ANY) },
                            };

    if (bind(udpSocket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return {1, udpSocket};
    }

    return {0, udpSocket};
}