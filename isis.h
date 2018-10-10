//
// Created by Linghan Xing on 10/1/18.
//

#ifndef ISIS_LAMPORT_ISIS_H
#define ISIS_LAMPORT_ISIS_H

#include <stdlib.h>
#include <fstream>
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
#define TIME_OUT 5000000
#define BUFFER_SIZE (2 * sizeof(SeqMessage))
#define DUMMY_DATA 1;

struct CachedMsg {
    uint32_t data;
    uint32_t message_id;
    uint32_t sender_id;
    uint32_t sequence_num;
    uint32_t proposer;
    bool deliverable;
};

enum state {
    establishing_connection,
    sending_data_msg,
    receiving_msg
};

enum msg_type {
    data,
    ack,
    seq,
    mk,
    unknown
};

class ISIS {
protected:
    uint32_t my_id; // current process's id
    uint32_t msg_count; // number of message that needs to be delivered
    uint32_t counter; // number of message that has been sent
    uint32_t curr_seq; // sequence number
    int marker;
    int listening_fd; // file descriptor
    state curr_state; // an enum to keep track of current state
    // a flag to indicate if its allowed to send msg;
    bool isblocked;
    bool recording;
    // to keep track of elapsed time
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    std::string port; // port number
    // past_msg is indicated by < sender_id, msg_id >
    std::vector<std::tuple<int, int>> past_msgs;
    // map of message -> (proposer, proposed_seq)
    std::unordered_map<int, std::unordered_map<int, int >> proposals;
    // a msg_q is a queue of CachedMsg
    std::vector<CachedMsg> msg_q;
    std::vector<std::string> addr_book;
    std::vector<CachedMsg> msg_delivered;

    long long int calc_elapsed_time();
    void broadcast_msg_to_timeout_nodes();
    void broadcast_marker();
    void make_local_snapshot();
    MkMessage* generate_marker_msg();
    void init();
    void broadcast_data_msg();
    void recv_msg();
    int num_of_nodes; // total number of processes
    void increment_seq();
    bool send_msg(void *msg, std::string addr, uint32_t size);
    msg_type check_msg_type(void * msg, ssize_t size);
    bool has_duplication(DataMessage *msg);
    void send_ack_msg(DataMessage* msg);
    void enque_msg(DataMessage* msg);
    void handle_q_change();
    void deliver_msg(CachedMsg *msg);
    uint32_t get_final_seq();
    void broadcast_final_seq(SeqMessage* msg);
    void establish_connection();
    void assess_next_state();
    SeqMessage * generate_seq_msg(AckMessage* msg);
    DataMessage* generate_data_msg();
    AckMessage * generate_ack_msg(DataMessage* msg);
    CachedMsg* find_msg(uint32_t msg_id, uint32_t sender_id);
    void handle_seq_msg(SeqMessage* msg);
    bool ack_has_received(AckMessage* msg);

public:
    ISIS(std::vector<std::string> &, std::string, int, int);
    ~ISIS();
    void run_isis();
};

#endif //ISIS_LAMPORT_ISIS_H
