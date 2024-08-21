#pragma once
#include <cstdint>
#include <vector>

class __attribute__((packed)) DNS_Message_Header {
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

class __attribute__((packed)) DNS_Message_Question {
public:
    uint16_t TYPE;
    uint16_t CLASS;
    DNS_Message_Question();
    void to_network_order();
};

class __attribute__((packed)) DNS_Message_Answer {
public:
    uint16_t TYPE;
    uint16_t CLASS;
    uint32_t TTL;
    uint16_t RDLENGTH;
    uint32_t RDATA;
    DNS_Message_Answer();
    void to_network_order();
};

class DNS_Message {
public:
    DNS_Message_Header header;
    std::vector<DNS_Message_Question> questions;
    std::vector<DNS_Message_Answer> answers;
    std::vector<std::vector<uint8_t>> labels;
    DNS_Message();
    void to_network_order();
};