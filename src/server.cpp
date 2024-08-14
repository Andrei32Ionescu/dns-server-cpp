#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include "dns_message.hpp"

enum FLAGS {
    RESPONSE_FLAG = (1 << 15),
};

void create_response(DNS_Message& dns_message) {
    dns_message.header.ID = 1234;
    dns_message.header.FLAGS = RESPONSE_FLAG;
    dns_message.header.QDCOUNT = 1;
    dns_message.header.ANCOUNT = 0;
    dns_message.header.NSCOUNT = 0;
    dns_message.header.ARCOUNT = 0;

    std::string first_domain = "codecrafters";
    dns_message.question.NAME.push_back((uint8_t)first_domain.size());
    for(char c : first_domain) {
        dns_message.question.NAME.push_back(c);
    }

    std::string second_domain = "io";
    dns_message.question.NAME.push_back((uint8_t)second_domain.size());
    for(char c : second_domain) {
        dns_message.question.NAME.push_back(c);
    }
    dns_message.question.NAME.push_back(0);
    
    dns_message.question.CLASS = 1;
    dns_message.question.TYPE = 1;

    dns_message.to_network_order();
}


int main() {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Disable output buffering
    setbuf(stdout, NULL);

    int udpSocket;
    struct sockaddr_in clientAddress;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "..." << std::endl;
        return 1;
    }

    // Ensures that 'Address already in use' errors are not encountered during testing
    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "SO_REUSEPORT failed: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serv_addr = { .sin_family = AF_INET,
                                .sin_port = htons(2053),
                                .sin_addr = { htonl(INADDR_ANY) },
                            };

    if (bind(udpSocket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    int bytesRead;
    char buffer[512];
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
        DNS_Message response;
        create_response(response);
        
        char responseBuffer[sizeof(response.header) + response.question.NAME.size()  + 4];
        std::copy((const char*) &response.header, (const char*) &response.header + sizeof(response.header), responseBuffer);
        std::copy(response.question.NAME.begin(), response.question.NAME.end(), responseBuffer + sizeof(response.header));
        responseBuffer[sizeof(response.header) + response.question.NAME.size()] = 0;
        responseBuffer[sizeof(response.header) + response.question.NAME.size() + 1] = 1;
        responseBuffer[sizeof(response.header) + response.question.NAME.size() + 2] = 0;
        responseBuffer[sizeof(response.header) + response.question.NAME.size() + 3] = 1;
        
        // Send response
        if (sendto(udpSocket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}
