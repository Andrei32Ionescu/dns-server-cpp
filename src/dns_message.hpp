#pragma once
#include <cstdint>
#include <vector>
#include <memory>

enum FLAGS {
    QR_FLAG = (1 << 15),
    OPCODE_FLAG = (15 << 11),
    RCODE_FLAG = (4),
    RD_FLAG = (1 << 8),
};


class __attribute__((packed)) DNS_Message_Header {
public:
    uint16_t ID;
    uint16_t FLAGS;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;

    DNS_Message_Header(uint16_t ID, uint16_t FLAGS, uint16_t QDCOUNT, uint16_t ANCOUNT, uint16_t NSCOUNT, uint16_t ARCOUNT);
    void to_network_order();
};

class __attribute__((packed)) DNS_Message_Question {
public:
    uint16_t TYPE;
    uint16_t CLASS;
    DNS_Message_Question(uint16_t TYPE, uint16_t CLASS);
    void to_network_order();
};

class __attribute__((packed)) DNS_Message_Answer {
public:
    uint16_t TYPE;
    uint16_t CLASS;
    uint32_t TTL;
    uint16_t RDLENGTH;
    uint32_t RDATA;
    DNS_Message_Answer(uint16_t TYPE, uint16_t CLASS, uint32_t TTL, uint16_t RDLENGTH, uint32_t RDATA);
    void to_network_order();
};

class DNS_Message {
public:
    DNS_Message_Header header;
    std::vector<std::unique_ptr<DNS_Message_Question>> questions;
    std::vector<std::unique_ptr<DNS_Message_Answer>> answers;
    std::vector<std::unique_ptr<std::vector<uint8_t>>> labels;
    DNS_Message(DNS_Message_Header header);
    void to_network_order();
    void create_response_labels(int question_number, uint8_t buffer[]);
};

DNS_Message create_response(uint8_t buffer[]);