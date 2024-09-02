#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "server_init.hpp"
#include "dns_message.hpp"
#include "request_handling.hpp"

int main(int argc, char** argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Disable output buffering
    setbuf(stdout, NULL);

    Resolver_Info resolver_info;
    std::string resolver;
    for (int i = 1; i < argc; i++) {
        if(i + 1 != argc) {
            if(strcmp(argv[i], "--resolver") == 0) {
                resolver = argv[i + 1];
                resolver_info.ip = resolver.substr(0, resolver.find(':'));
                resolver_info.port = resolver.substr(resolver.find(':') + 1);
                break;
            }
        }
    }
    setup_resolver_socket(resolver_info);

    // First element of the pair is the code returned during server creation
    // Second element of the pair is the resulting socket file descriptor
    std::pair<int, int> server_status = create_server(2053);
    if(server_status.first != 0) {
        std::cerr<< "Server creation failed!\n";
        return server_status.first;
    }
    int udpSocket = server_status.second;
    struct sockaddr_in clientAddress;

    int bytesRead;
    uint8_t buffer[512];
    socklen_t clientAddrLen = sizeof(clientAddress);

    while (true) {
        // Receive data
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received " << bytesRead << " bytes: " << buffer << std::endl;

        // Create the response
        DNS_Message response = create_response(buffer);
        int question_number = response.header.QDCOUNT;
        response.to_network_order();
        response.create_response_labels(question_number, buffer);

        // Forward message to resolver if possible
        int question_index{0};
        while(!resolver_info.ip.empty() && question_index < question_number) {
            query_resolver_server(resolver_info.socket, resolver_info.addr, response, question_index);
            question_index++;
        }

        std::pair<std::unique_ptr<uint8_t[]>, size_t> responseBuffer = create_response_buffer(question_number, response);

        // Send response
        if (sendto(udpSocket, responseBuffer.first.get(), responseBuffer.second, 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);
    close(resolver_info.socket);

    return 0;
}