#pragma once
#include "dns_message.hpp"
#include <netinet/in.h>

void query_resolver_server(int resolver_socket, sockaddr_in resolver_addr, DNS_Message& response, int question_index);
std::pair<std::unique_ptr<uint8_t[]>, size_t> create_response_buffer(int question_number, const DNS_Message& response);