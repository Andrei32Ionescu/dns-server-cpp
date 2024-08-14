#include <arpa/inet.h>
#include "dns_message.hpp"

DNS_Message_Header::DNS_Message_Header() {}

DNS_Message_Question::DNS_Message_Question() {}

DNS_Message::DNS_Message() {
    DNS_Message_Header header;
    DNS_Message_Question question;
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

void DNS_Message::to_network_order() {
    header.to_network_order();
    question.to_network_order();
}