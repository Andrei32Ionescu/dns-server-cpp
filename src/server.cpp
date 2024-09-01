#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <bitset>
#include <arpa/inet.h>
#include "dns_message.hpp"

enum FLAGS {
    QR_FLAG = (1 << 15),
    OPCODE_FLAG = (15 << 11),
    RCODE_FLAG = (4),
    RD_FLAG = (1 << 8),
};

DNS_Message create_response(uint8_t buffer[]) {
    uint16_t header_ID = uint16_t ((buffer[0] << 8) | buffer[1]);
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

    for(int i = 0; i < dns_message.header.QDCOUNT; i++) {
        DNS_Message_Question* question = new DNS_Message_Question();
        question -> CLASS = 1;
        question -> TYPE = 1;
        dns_message.questions.push_back(*question);
    }

    for(int i = 0; i < dns_message.header.ANCOUNT; i++) {
        DNS_Message_Answer* answer = new DNS_Message_Answer();
        answer -> CLASS = 1;
        answer -> TYPE = 1;
        answer -> TTL = 60;
        answer -> RDLENGTH = 4;
        answer -> RDATA = 0x08080808;
        dns_message.answers.push_back(*answer);
    }

    return dns_message;
}

void query_resolver(int resolver_socket, sockaddr_in resolver_addr, DNS_Message& response, int question_index) {
    DNS_Message_Header query_header = response.header;
    query_header.ANCOUNT = 0;
    query_header.QDCOUNT = htons(1);
    query_header.FLAGS = htons(ntohs(query_header.FLAGS) & (~QR_FLAG));

    // Calculate response size
    int response_size = sizeof(query_header);
    response_size += sizeof(response.questions[question_index]);
    response_size += response.labels[question_index].size();

    // Create and populate the buffer 
    uint8_t responseBuffer[response_size];
    std::copy((const char*) &query_header, (const char*) &query_header + sizeof(response.header), responseBuffer);
    size_t curr_index = sizeof(query_header);
    std::copy(response.labels[question_index].begin(), response.labels[question_index].end(), responseBuffer + curr_index);
    curr_index += response.labels[question_index].size();
    std::copy((const char*) &response.questions[question_index], (const char*) &response.questions[question_index] + sizeof(response.questions[question_index]), responseBuffer + curr_index);
    curr_index += sizeof(response.questions[question_index]);

    // Send response
    if (sendto(resolver_socket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&resolver_addr), sizeof(resolver_addr)) == -1) {
        perror("Failed to send response");
    }

    int bytesRead;
    uint8_t buffer[512];
    socklen_t resolver_addrLen = sizeof(resolver_addr);
    
    // Receive data
    bytesRead = recvfrom(resolver_socket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&resolver_addr), &resolver_addrLen);
    if (bytesRead == -1) {
        perror("Error receiving data");
    }

    if (buffer[curr_index + response.labels[question_index].size()] == 0 && 
    buffer[curr_index + response.labels[question_index].size() + 1] == 1 &&
    buffer[curr_index + response.labels[question_index].size() + 2] == 0 &&
    buffer[curr_index + response.labels[question_index].size() + 3] == 1) {
        std::memcpy(&response.answers[question_index], buffer + curr_index + response.labels[question_index].size(), sizeof(response.answers[question_index]));
    }
}

int main(int argc, char** argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Disable output buffering
    setbuf(stdout, NULL);

    std::string resolver, resolver_ip, resolver_port;
    for (int i = 1; i < argc; i++) {
        if(i + 1 != argc) {
            if(strcmp(argv[i], "--resolver") == 0) {
                resolver = argv[i + 1];
                resolver_ip = resolver.substr(0, resolver.find(':'));
                resolver_port = resolver.substr(resolver.find(':') + 1);
                break;
            }
        }
    }

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

    sockaddr_in resolver_addr;
    int resolver_socket;
    if(resolver_ip.size() > 0) {
        resolver_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (resolver_socket == -1) {
            std::cerr << "Socket creation failed: " << strerror(errno) << "..." << std::endl;
            return 1;
        }

        resolver_addr = { 
            .sin_family = AF_INET,
            .sin_port = htons(std::stoi(resolver_port)),
        };
        if (inet_pton(AF_INET, resolver_ip.c_str(), &resolver_addr.sin_addr) <= 0) {
            std::cerr << "Invalid resolver IP address" << std::endl;
        }
    }

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
        int index{12};
        for(int i = 0; i < question_number; i++) {
            std::vector<uint8_t>* label = new std::vector<uint8_t>;

            while(buffer[index] != '\x00') {
                uint8_t first_two_bits = buffer[index] >> 6;
                uint16_t address = index;
                if(first_two_bits == 0b00000011) {
                    while(first_two_bits == 0b00000011) {
                        address = (buffer[address] << 8 | buffer[address + 1]) << 2;
                        address = address >> 2;
                        first_two_bits = buffer[address] >> 6;
                    }

                    while(buffer[address] != '\x00') {
                        label -> push_back(buffer[address++]);
                    }
                    index++;
                    break;
                }
                else {
                    label -> push_back(buffer[index++]);
                }
            }

            label -> push_back(0);
            index += 5;

            response.labels.push_back(*label);
        }

        // Forward message to resolver if possible
        int question_index{0};
        while(!resolver_ip.empty() && question_index < question_number) {
            query_resolver(resolver_socket, resolver_addr, response, question_index);
            question_index++;
        }

        // Calculate response size
        int response_size{0};
        response_size += sizeof(response.header);
        for(int i = 0; i < question_number; i++) {
            response_size += sizeof(response.questions[i]);
            response_size += sizeof(response.answers[i]);
            response_size += 2 * response.labels[i].size();
        }
        
        // Create and populate the response buffer 
        uint8_t responseBuffer[response_size];
        std::copy((const char*) &response.header, (const char*) &response.header + sizeof(response.header), responseBuffer);
        size_t curr_index = sizeof(response.header);

        for(int i = 0; i < question_number; i++) {
            std::copy(response.labels[i].begin(), response.labels[i].end(), responseBuffer + curr_index);
            curr_index += response.labels[i].size();
            std::copy((const char*) &response.questions[i], (const char*) &response.questions[i] + sizeof(response.questions[i]), responseBuffer + curr_index);
            curr_index += sizeof(response.questions[i]);      
        }

        for(int i = 0; i < question_number; i++) {
            std::copy(response.labels[i].begin(), response.labels[i].end(), responseBuffer + curr_index);
            curr_index += response.labels[i].size();   
            std::copy((const char*) &response.answers[i], (const char*) &response.answers[i] + sizeof(response.answers[i]), responseBuffer + curr_index);
            curr_index += sizeof(response.answers[i]);
        }

        // Send response
        if (sendto(udpSocket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}