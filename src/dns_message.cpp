#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <bitset>
#include <arpa/inet.h>
#include "dns_message.hpp"

DNS_Message_Header::DNS_Message_Header(uint16_t ID, uint16_t FLAGS, uint16_t QDCOUNT, uint16_t ANCOUNT, uint16_t NSCOUNT, uint16_t ARCOUNT) : 
ID(ID), FLAGS(FLAGS), QDCOUNT(QDCOUNT), ANCOUNT(ANCOUNT), NSCOUNT(NSCOUNT), ARCOUNT(ARCOUNT) {}

DNS_Message_Question::DNS_Message_Question(uint16_t TYPE, uint16_t CLASS) : TYPE(TYPE), CLASS(CLASS) {}

DNS_Message_Answer::DNS_Message_Answer(uint16_t TYPE, uint16_t CLASS, uint32_t TTL, uint16_t RDLENGTH, uint32_t RDATA) :
TYPE(TYPE), CLASS(CLASS), TTL(TTL), RDLENGTH(RDLENGTH), RDATA(RDATA) {}

DNS_Message::DNS_Message(DNS_Message_Header header) : header(header) {
    std::vector<std::unique_ptr<DNS_Message_Question>> questions;
    std::vector<std::unique_ptr<DNS_Message_Answer>> answers;
    std::vector<std::unique_ptr<std::vector<uint8_t>>> labels;
}

void DNS_Message_Header::to_network_order() {
    ID      = htons(ID);
    FLAGS   = htons(FLAGS);
    QDCOUNT = htons(QDCOUNT);
    ANCOUNT = htons(ANCOUNT);
    NSCOUNT = htons(NSCOUNT);
    ARCOUNT = htons(ARCOUNT);
}

void DNS_Message_Question::to_network_order() {
    TYPE = htons(TYPE);
    CLASS = htons(CLASS);
}

void DNS_Message_Answer::to_network_order() {
    TYPE     = htons(TYPE);
    CLASS    = htons(CLASS);
    TTL      = htons(TTL);
    RDLENGTH = htons(RDLENGTH);
    RDATA    = htons(RDATA);
}

void DNS_Message::to_network_order() {
    header.to_network_order();

    for(int i = 0; i < questions.size(); i++) {
        questions[i] -> to_network_order();
    }
    
    for(int i = 0; i < answers.size(); i++) {
        answers[i] -> to_network_order();
    }
}

void DNS_Message::create_response_labels(int question_number, uint8_t buffer[]) {
    int index{12};
    for(int i = 0; i < question_number; i++) {
        labels.push_back(std::make_unique<std::vector<uint8_t>>());

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
                    labels.back() -> push_back(buffer[address++]);
                }
                index++;
                break;
            }
            else {
                labels.back() -> push_back(buffer[index++]);
            }
        }

        labels.back() -> push_back(0);
        index += 5;
    }
}

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

    uint16_t ID = header_ID;
    uint16_t FLAGS = response_flag;
    uint16_t QDCOUNT = uint16_t ((buffer[4] << 8) | buffer[5]);
    uint16_t ANCOUNT = QDCOUNT;
    uint16_t NSCOUNT = 0;
    uint16_t ARCOUNT = 0;
    DNS_Message dns_message = DNS_Message(DNS_Message_Header(ID, FLAGS, QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT));

    uint16_t TYPE  = 1;
    uint16_t CLASS = 1;
    for(int i = 0; i < QDCOUNT; i++) {
        dns_message.questions.push_back(std::make_unique<DNS_Message_Question>(TYPE, CLASS));
    }

    uint32_t TTL = 60;
    uint16_t RDLENGTH = 4;
    uint32_t RDATA = 0x08080808;
    for(int i = 0; i <ANCOUNT; i++) {
       dns_message.answers.push_back(std::make_unique<DNS_Message_Answer>(CLASS, TYPE, TTL, RDLENGTH, RDATA));
    }

    return dns_message;
}