#pragma once
#include <cstdint>
#include <vector>

class DNS_Message_Header {
public:
    uint16_t ID;
    uint16_t FLAGS;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;

    DNS_Message_Header();
    void to_network_order();
};

class DNS_Message_Question {
public:
    std::vector<char> NAME;
    uint16_t TYPE;
    uint16_t CLASS;
    DNS_Message_Question();
    void to_network_order();
};
class DNS_Message {
public:
    DNS_Message_Header header;
    DNS_Message_Question question;
    DNS_Message();
    void to_network_order();
};