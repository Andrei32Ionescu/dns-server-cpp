#include <cstdint>
#include <iostream>
#include <cstring>
#include "request_handling.hpp"

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
        std::cerr << "Failed to send response" << std::endl;
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

std::pair<std::unique_ptr<uint8_t[]>, size_t> create_response_buffer(int question_number, const DNS_Message& response) {
    // Calculate response size
    int response_size{0};
    response_size += sizeof(response.header);
    for(int i = 0; i < question_number; i++) {
        response_size += sizeof(*response.questions[i]);
        response_size += sizeof(*response.answers[i]);
        response_size += 2 * response.labels[i] -> size();
    }
    
    // Create and populate the response buffer 
    std::unique_ptr<uint8_t[]> responseBuffer = std::make_unique<uint8_t[]>(response_size);

    std::copy((const char*) &response.header, (const char*) &response.header + sizeof(response.header), responseBuffer.get());
    size_t curr_index = sizeof(response.header);

    for(int i = 0; i < question_number; i++) {
        std::copy(response.labels[i] -> begin(), response.labels[i] -> end(), responseBuffer.get() + curr_index);
        curr_index += response.labels[i] -> size();
        std::copy((const char*) response.questions[i].get(), (const char*) response.questions[i].get() + sizeof(*response.questions[i]), responseBuffer.get() + curr_index);
        curr_index += sizeof(*response.questions[i]);      
    }

    for(int i = 0; i < question_number; i++) {
        std::copy(response.labels[i] -> begin(), response.labels[i] -> end(), responseBuffer.get() + curr_index);
        curr_index += response.labels[i] -> size();   
        std::copy((const char*) response.answers[i].get(), (const char*) response.answers[i].get() + sizeof(*response.answers[i]), responseBuffer.get() + curr_index);
        curr_index += sizeof(*response.answers[i]);
    }

    return {std::move(responseBuffer), response_size};
}