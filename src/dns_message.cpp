#include <arpa/inet.h>
#include "dns_message.hpp"

DNS_Message::DNS_Message() {}

void DNS_Message::to_network_order() {
    ID      = htons(ID);
    FLAGS   = htons(FLAGS);
    QDCOUNT = htons(QDCOUNT);
    ANCOUNT = htons(ANCOUNT);
    NSCOUNT = htons(NSCOUNT);
    ARCOUNT = htons(ARCOUNT);
}