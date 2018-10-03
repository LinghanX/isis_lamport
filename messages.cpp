//
// Created by Linghan Xing on 10/3/18.
//
#include "messages.h"

DataMessage* hton(DataMessage* msg) {
    msg -> type = htonl(msg->type);
    msg -> sender = htonl(msg -> sender);
    msg -> msg_id = htonl(msg -> msg_id);
    msg -> data = htonl(msg -> data);

    return msg;
}
// here we cannot call ntohl(msg -> type) since it has already been converted
DataMessage* ntoh(DataMessage* msg) {
    msg -> sender = ntohl(msg -> sender);
    msg -> msg_id = ntohl(msg -> msg_id);
    msg -> data = ntohl(msg -> data);
    return msg;
}
AckMessage* hton(AckMessage* msg) {
    msg -> type = htonl(msg -> type);
    msg -> sender = htonl(msg -> sender);
    msg -> msg_id = htonl(msg -> msg_id);
    msg -> proposed_seq = htonl(msg -> proposed_seq);
    msg -> proposer = htonl(msg -> proposer);

    return msg;
}
AckMessage* ntoh(AckMessage* msg) {
    msg -> sender = ntohl(msg -> sender);
    msg -> msg_id = ntohl(msg -> msg_id);
    msg -> proposed_seq = ntohl(msg -> proposed_seq);
    msg -> proposer = ntohl(msg -> proposer);
    return msg;
}
SeqMessage* ntoh(SeqMessage* msg) {
    msg -> sender = ntohl(msg -> sender);
    msg -> msg_id = ntohl(msg -> msg_id);
    msg -> final_seq = ntohl(msg -> final_seq);
    msg -> final_seq_proposer = ntohl(msg -> final_seq_proposer);
    return msg;
}
SeqMessage* hton(SeqMessage* msg) {
    msg -> type = htonl(msg -> type);
    msg -> sender = htonl(msg -> sender);
    msg -> msg_id = htonl(msg -> msg_id);
    msg -> final_seq = htonl(msg -> final_seq);
    msg -> final_seq_proposer = htonl(msg -> final_seq_proposer);
    return msg;
}