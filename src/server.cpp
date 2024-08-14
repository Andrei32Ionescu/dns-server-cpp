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
    dns_message.header.ANCOUNT = 1;
    dns_message.header.NSCOUNT = 0;
    dns_message.header.ARCOUNT = 0;

    std::string first_domain = "codecrafters";
    int index{0};
    dns_message.question.NAME[index++] = (uint8_t)first_domain.size();
    for(char c : first_domain) {
        dns_message.question.NAME[index] = c;
        index++;
    }

    std::string second_domain = "io";
    dns_message.question.NAME[index++] = (uint8_t)second_domain.size();
    for(char c : second_domain) {
        dns_message.question.NAME[index] = c;
        index++;
    }
    dns_message.question.NAME[index] = 0;
    
    dns_message.question.CLASS = 1;
    dns_message.question.TYPE = 1;

    std::copy(dns_message.question.NAME, dns_message.question.NAME + sizeof(dns_message.question.NAME), dns_message.answer.NAME);
    dns_message.answer.TYPE = 1;
    dns_message.answer.CLASS = 1;
    dns_message.answer.TTL = 60;
    dns_message.answer.RDLENGTH = 4;
    dns_message.answer.RDATA = 8888;
    
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

        char responseBuffer[sizeof(response)];
        std::copy((const char*) &response, (const char*) &response + sizeof(response), responseBuffer);
        
        // Send response
        if (sendto(udpSocket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}
