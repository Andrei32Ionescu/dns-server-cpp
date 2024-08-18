#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <bitset>
#include "dns_message.hpp"

enum FLAGS {
    QR_FLAG = (1 << 15),
    OPCODE_FLAG = (15 << 11),
    RCODE_FLAG = (4),
    RD_FLAG = (1 << 8),
};

DNS_Message create_response(char buffer[]) {
    uint16_t header_ID = uint16_t ((uint16_t (buffer[0] << 8) | buffer[1]));
    uint16_t request_flags = uint16_t ((buffer[2] << 8) | buffer[3]);
    uint16_t opcode = (request_flags << 1) >> 12;
    uint16_t response_flag = QR_FLAG | (opcode << 11);
    
    if(request_flags & RD_FLAG) {
        response_flag |= RD_FLAG;
    }   
    if(opcode != 0) {
        response_flag = response_flag | RCODE_FLAG;
    }

    DNS_Message dns_message;
    dns_message.header.ID = header_ID;
    dns_message.header.FLAGS = response_flag;
    dns_message.header.QDCOUNT = uint16_t ((buffer[4] << 8) | buffer[5]);
    dns_message.header.ANCOUNT = dns_message.header.QDCOUNT;
    dns_message.header.NSCOUNT = 0;
    dns_message.header.ARCOUNT = 0;

    dns_message.question.CLASS = 1;
    dns_message.question.TYPE = 1;

    dns_message.answer.TYPE = 1;
    dns_message.answer.CLASS = 1;
    dns_message.answer.TTL = 60;
    dns_message.answer.RDLENGTH = 4;
    dns_message.answer.RDATA = 8888;
    
    //dns_message.to_network_order();
    return dns_message;
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
        DNS_Message response = create_response(buffer);

        int index{12}, i{0}, j{0};
        char labels [256];
        while(j < response.header.QDCOUNT) {
            while(buffer[index] != '\x00') {
                labels[i++] = buffer[index++];
            }
            labels[i++] = 0;
            j++;
        }

        response.to_network_order();
        uint8_t responseBuffer[sizeof(response) + 2*i];
        std::copy((const char*) &response.header, (const char*) &response.header + sizeof(response.header), responseBuffer);
        std::copy((const char*) &labels, (const char*) &labels + i, responseBuffer + sizeof(response.header));
        std::copy((const char*) &response.question, (const char*) &response.question + sizeof(response.question), responseBuffer + sizeof(response.header) + i);
        std::copy((const char*) &labels, (const char*) &labels + i,  responseBuffer + sizeof(response.header) + i + sizeof(response.question));
        std::copy((const char*) &response.answer, (const char*) &response.answer + sizeof(response.answer), responseBuffer + sizeof(response.header) + 2* i + sizeof(response.question));
        // TODO: find out why copying the ID using the above method is wrong and the below method is correct
        std::copy(buffer, buffer + 2, responseBuffer);

        // Send response
        if (sendto(udpSocket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}