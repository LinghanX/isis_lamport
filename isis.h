//
// Created by Linghan Xing on 10/1/18.
//

#ifndef ISIS_LAMPORT_ISIS_H
#define ISIS_LAMPORT_ISIS_H

#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include "messages.h"

#define MAX_HOSTNAME_LEN 256
#define MAX_SOCK 128;

struct CachedMsg {
    uint32_t data;
    uint32_t message_id;
    uint32_t sender_id;
    uint32_t sequence_num;
    uint32_t my_id;
    bool deliverable;
};

class ISIS {
protected:
    int my_id; // current process's id
    int msg_count; // number of message that needs to be delivered
    int msg_sent; // number of message that has been sent
    int listening_fd; // file descriptor

    // a flag to indicate if its allowed to send msg;
    bool isblocked;

    std::string port; // port number
    std::vector<int> seq; // a vector of sequence num
    std::vector<bool> ack; // a vector of ackowledgement
    // a msg_q is a queue of CachedMsg
    std::vector<CachedMsg> msg_q;
    std::unordered_map<std::string, int> hostname_to_id;
    // a map that stores message_id -> (sequence number, proposer id)
    std::unordered_map<int, std::vector<std::tuple<int, int>>> map;

    void init();


public:
    ISIS(const std::unordered_map<std::string, int>&, std::string, int);
    ~ISIS();
    void broadcast();
};

#endif //ISIS_LAMPORT_ISIS_H
