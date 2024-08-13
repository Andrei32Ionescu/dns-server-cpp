#pragma once
#include <cstdint>

class DNS_Message {
public:
    uint16_t ID;
    uint16_t FLAGS;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;

    DNS_Message();
    void to_network_order();
};