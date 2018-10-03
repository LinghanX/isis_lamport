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
#include <chrono>
#include "messages.h"
#include <ctime>
#include <sys/time.h>

#define MAX_HOSTNAME_LEN 256
#define MAX_SOCK 128
// in microseconds
#define TIME_OUT 1000000
#define BUFFER_SIZE (2 * sizeof(SeqMessage))
#define DUMMY_DATA 1;

struct CachedMsg {
    uint32_t data;
    uint32_t message_id;
    uint32_t sender_id;
    uint32_t sequence_num;
    uint32_t my_id;
    bool deliverable;
};

enum state {
    sending_data_msg,
    waiting_ack,
    waiting_seq
};

enum msg_type {
    data,
    ack,
    seq
};

class ISIS {
protected:
    uint32_t my_id; // current process's id
    int msg_count; // number of message that needs to be delivered
    int msg_sent; // number of message that has been sent
    int listening_fd; // file descriptor
    state curr_state; // an enum to keep track of current state

    // a flag to indicate if its allowed to send msg;
    bool isblocked;

    std::string port; // port number
    std::vector<int> seq; // a vector of sequence num
    std::vector<bool> ack; // a vector of ackowledgement
    // a msg_q is a queue of CachedMsg
    std::vector<CachedMsg> msg_q;
    std::unordered_map<std::string, int> hostname_to_id;
    std::vector<std::string> addr_book;
    // a map that stores message_id -> (sequence number, proposer id)
    std::unordered_map<int, std::vector<std::tuple<int, int>>> map;
    void init();
    void broadcast_data_msg();
    void recv_ack();
    void recv_seq();
    int num_of_nodes; // total number of processes
    void increment_seq();
    bool send_msg(void *msg, std::string addr, uint32_t size);

    DataMessage* generate_data_msg();

public:
    ISIS(std::vector<std::string> &, std::string, int);
    ~ISIS();
    void start();
};

#endif //ISIS_LAMPORT_ISIS_H
