#include <arpa/inet.h>
#include "dns_message.hpp"

DNS_Message_Header::DNS_Message_Header() {}

DNS_Message_Question::DNS_Message_Question() {}

DNS_Message_Answer::DNS_Message_Answer() {}

DNS_Message::DNS_Message() {
    DNS_Message_Header header;
    std::vector<DNS_Message_Question> questions;
    std::vector<DNS_Message_Answer> answers;
    std::vector<std::vector<uint8_t>> labels;
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
    CLASS = htons(CLASS);
    TYPE = htons(TYPE);
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
        questions[i].to_network_order();
    }
    
     for(int i = 0; i < answers.size(); i++) {
        answers[i].to_network_order();
    }
}