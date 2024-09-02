#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <bitset>
#include <arpa/inet.h>
#include "server_init.hpp"
#include "dns_message.hpp"

enum FLAGS {
    QR_FLAG = (1 << 15),
    OPCODE_FLAG = (15 << 11),
    RCODE_FLAG = (4),
    RD_FLAG = (1 << 8),
};

void query_resolver_server(int resolver_socket, sockaddr_in resolver_addr, DNS_Message& response, int question_index) {
    DNS_Message_Header query_header = response.header;
    query_header.ANCOUNT = 0;
    query_header.QDCOUNT = htons(1);
    query_header.FLAGS = htons(ntohs(query_header.FLAGS) & (~QR_FLAG));

    // Calculate response size
    int response_size = sizeof(query_header);
    response_size += sizeof(*response.questions[question_index]);
    response_size += response.labels[question_index] -> size();

    // Create and populate the buffer 
    uint8_t responseBuffer[response_size];
    std::copy((const char*) &query_header, (const char*) &query_header + sizeof(response.header), responseBuffer);
    size_t curr_index = sizeof(query_header);
    std::copy(response.labels[question_index] -> begin(), response.labels[question_index] -> end(), responseBuffer + curr_index);
    curr_index += response.labels[question_index] -> size();
    std::copy((const char*) response.questions[question_index].get(), (const char*) response.questions[question_index].get() + sizeof(response.questions[question_index]), responseBuffer + curr_index);
    curr_index += sizeof(*response.questions[question_index]);

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

    if (buffer[curr_index + response.labels[question_index] -> size()] == 0 && 
    buffer[curr_index + response.labels[question_index] -> size() + 1] == 1 &&
    buffer[curr_index + response.labels[question_index] -> size() + 2] == 0 &&
    buffer[curr_index + response.labels[question_index] -> size() + 3] == 1) {
        std::memcpy(response.answers[question_index].get(), buffer + curr_index + response.labels[question_index] -> size(), sizeof(*response.answers[question_index]));
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

    // First element of the pair is the code returned during server creation
    // Second element of the pair is the resulting socket file descriptor
    std::pair<int, int> server_status = create_server(2053);
    if(server_status.first != 0) {
        std::cerr<< "Server creation failed!\n";
        return server_status.first;
    }
    int udpSocket = server_status.second;
    struct sockaddr_in clientAddress;

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
        response.create_response_labels(question_number, buffer);

        // Forward message to resolver if possible
        int question_index{0};
        while(!resolver_ip.empty() && question_index < question_number) {
            query_resolver_server(resolver_socket, resolver_addr, response, question_index);
            question_index++;
        }

        // Calculate response size
        int response_size{0};
        response_size += sizeof(response.header);
        for(int i = 0; i < question_number; i++) {
            response_size += sizeof(*response.questions[i]);
            response_size += sizeof(*response.answers[i]);
            response_size += 2 * response.labels[i] -> size();
        }
        
        // Create and populate the response buffer 
        uint8_t responseBuffer[response_size];
        std::copy((const char*) &response.header, (const char*) &response.header + sizeof(response.header), responseBuffer);
        size_t curr_index = sizeof(response.header);

        for(int i = 0; i < question_number; i++) {
            std::copy(response.labels[i] -> begin(), response.labels[i] -> end(), responseBuffer + curr_index);
            curr_index += response.labels[i] -> size();
            std::copy((const char*) response.questions[i].get(), (const char*) response.questions[i].get() + sizeof(*response.questions[i]), responseBuffer + curr_index);
            curr_index += sizeof(*response.questions[i]);      
        }

        for(int i = 0; i < question_number; i++) {
            std::copy(response.labels[i] -> begin(), response.labels[i] -> end(), responseBuffer + curr_index);
            curr_index += response.labels[i] -> size();   
            std::copy((const char*) response.answers[i].get(), (const char*) response.answers[i].get() + sizeof(*response.answers[i]), responseBuffer + curr_index);
            curr_index += sizeof(*response.answers[i]);
        }

        // Send response
        if (sendto(udpSocket, &responseBuffer, sizeof(responseBuffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);
    close(resolver_socket);

    return 0;
}