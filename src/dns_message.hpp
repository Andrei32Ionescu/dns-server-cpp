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
    //u_int8_t NAME[17];
    uint16_t TYPE;
    uint16_t CLASS;
    DNS_Message_Question();
    void to_network_order();
};

class __attribute__((packed)) DNS_Message_Answer {
public:
    //u_int8_t NAME[17];
    uint16_t TYPE;
    u_int16_t CLASS;
    u_int32_t TTL;
    uint16_t RDLENGTH;
    u_int32_t RDATA;
    DNS_Message_Answer();
    void to_network_order();
};

class __attribute__((packed)) DNS_Message {
public:
    DNS_Message_Header header;
    DNS_Message_Question question;
    DNS_Message_Answer answer;
    DNS_Message();
    void to_network_order();
};