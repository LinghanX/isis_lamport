//
// Created by Linghan Xing on 10/1/18.
//
#include "isis.h"

void ISIS::init() {
    auto logger = spdlog::get("console");
    struct addrinfo hints, *res, *res0;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // to allow unstable/reliable connection;
    hints.ai_flags = AI_PASSIVE;

    if ( (getaddrinfo(nullptr, port.c_str(), &hints, &res)) < 0) {
        logger -> error("not able to start listening");
    }

    for (res0 = res; res0 != nullptr; res0 = res0 -> ai_next) {
        if (
                (fd = socket(
                        res0 -> ai_family,
                        res0 -> ai_socktype,
                        res0 -> ai_protocol))
                < 0)
        {
           logger -> error("unable to create socket fd");
           continue;
        }

        if ( bind(fd, res0 -> ai_addr, res0 -> ai_addrlen) < 0 ) {
            close(listening_fd);
            logger -> error("unable to bind fd: {}", listening_fd);
            continue;
        }

        break;
    }

    if (res0 == nullptr) logger -> error("failed to create or bind socket");
    freeaddrinfo(res);
    this -> listening_fd = fd;

    logger -> info("start listening on port: {}, with fd: {}", port, fd);
}
ISIS::~ISIS() {
    close(this-> listening_fd);
}
ISIS::ISIS( std::vector<std::string> &addr_book,
            std::string port,
            int msg_num)
{
    auto logger = spdlog::get("console");
    char currHostName[MAX_HOSTNAME_LEN]; // my name
    if (gethostname(currHostName, MAX_HOSTNAME_LEN) < 0) {
        logger -> error("not able to get my host name");
    }

    int id = 0;
    bool find_id = false;
    for (auto const& addr : addr_book) {
        if (addr.compare(currHostName) == 0) {
            find_id = true;
            this -> my_id = static_cast<uint32_t>(id);
            break;
        } else id++;
    }

    if (!find_id) logger -> error("unable to parse my id");

    this -> port = port;
    this -> msg_count = static_cast<uint32_t>(msg_num);
    this -> addr_book = addr_book;
    this -> curr_state = establishing_connection;
    this -> num_of_nodes = static_cast<int>(addr_book.size());

    this -> curr_seq = 0;
    this -> counter = 0;
    init();
    logger -> info("initiating ISIS with curr_state: {}", this -> curr_state);
    logger -> info("initiating ISIS with msg count: {}", this -> msg_count);
    logger -> info("initiating ISIS with msg sent: {}", this -> counter);
    logger -> info("initiating ISIS with num of nodes: {}", this -> num_of_nodes);
    logger -> info("initiating ISIS with my id: {}", this -> my_id);
    for (const auto& n: addr_book) {
        logger -> info("initiating ISIS with addr_book item: {}", n);
    }
}
DataMessage* ISIS::generate_data_msg() {
    auto *msg = new DataMessage;
    msg -> type = 1;
    msg -> sender = this -> my_id;
    msg -> data = DUMMY_DATA;
    msg -> msg_id = static_cast<uint32_t>(this -> counter); // the msg_id is always num of msg sent

    return msg;
}
void ISIS::broadcast_data_msg() {
    const auto logger = spdlog::get("console");
    DataMessage *msg = generate_data_msg();
    if (msg == nullptr)
        logger -> error("unable to allocate data message");
    logger -> info("start broadcasting msg sender: {}, id: {}, type: {}", msg ->sender, msg ->msg_id, msg->type);
    int num_of_msg_send = 0;
    std::vector<bool> has_sent_msg(static_cast<unsigned long>(this -> num_of_nodes), false);

    this -> counter += 1;
    // send the message to myself;
    enque_msg(msg);
    // before we send, we need to convert message to network endian
    hton(msg);
    // seq.size() is the total num of id;
    while (num_of_msg_send < this -> num_of_nodes - 1) {
        for (uint32_t id = 0; id < this -> num_of_nodes; id++) {
            // nothing to do when its myself, or the msg has been sent
            if (id == this -> my_id || has_sent_msg[id]) continue;
            bool sent = this->send_msg(msg, this->addr_book[id], sizeof(DataMessage));
            if (sent) {
                num_of_msg_send++;
                has_sent_msg[id] = true;
                logger -> info("message has been sent to {}", this -> addr_book[id]);
            }
        }
    }
    delete(msg);

    logger -> info("all data message successfully sent");
    this -> isblocked = true; // blocked waiting for ackowledgement
}
bool ISIS::send_msg(void *msg, std::string addr, uint32_t size) {
    const auto logger = spdlog::get("console");
    logger -> info("start sending message to addr {}", addr);

    int sock_fd, status, num_bytes;
    struct addrinfo hint, *res, *res0;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_PASSIVE;

    if ( (status = getaddrinfo(addr.c_str(), this -> port.c_str(), &hint, &res)) < 0 ) {
        logger -> error("unable to get host address {}", addr);
        return false;
    }

    for (res0 = res; res0 != nullptr; res0 = res0 -> ai_next) {
        if ( (sock_fd = socket(
                res0 -> ai_family,
                res0 -> ai_socktype,
                res0 -> ai_protocol)) < 0 ) {
            logger -> error("unable to create the socket");
            return false;
        } else break;
    }

    if (res0 == nullptr) {
        logger -> error("unable to find socket pointer");
        return false;
    }

    if ( (num_bytes = sendto(sock_fd, msg, size, 0, res0 -> ai_addr, res0 -> ai_addrlen)) < 0 ) {
        logger -> error("unable to send msg");
        return false;
    }
    logger -> info("message successfully sent");

    close(sock_fd);
    freeaddrinfo(res);

    return true;
}
void ISIS::recv_msg() {
    const auto logger = spdlog::get("console");
//    logger -> info("start receiving message");
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;
    struct sockaddr_in neighbor;
    socklen_t addr_len = sizeof(neighbor);

    if ((num_bytes = recvfrom(
            this -> listening_fd,
            buffer,
            BUFFER_SIZE,
            MSG_DONTWAIT,
            (struct sockaddr *) &neighbor,
            &addr_len)) < 0 )
    {
//        logger -> error("unable to receive message");
    }
    if (num_bytes <= 0) return;

    msg_type type = check_msg_type(buffer, num_bytes);
    switch (type) {
        case msg_type::data:
        {
            DataMessage* msg = ntoh((DataMessage *) buffer);
            logger -> info("received data message id: {}, sender: {}",
                           msg ->msg_id, msg ->sender);
            if(!has_duplication(msg)) {
                this -> curr_seq += 1;
                send_ack_msg(msg);
                enque_msg(msg);
            }
            break;
        }
        case msg_type::ack:
        {
            AckMessage* ack_msg = ntoh((AckMessage *) buffer);

            logger -> info("received acknowledgement id: {}, sender: {}, propser: {}, proposed seq: {}",
                    ack_msg ->msg_id,
                    ack_msg ->sender,
                    ack_msg ->proposer,
                    ack_msg ->proposed_seq);
            if (ack_has_received(ack_msg)) {
                logger -> info("the ackowledgement has been received before");
            } else {
//                if (this -> proposals.count(ack_msg -> msg_id) == 0) {
//                    std::unordered_map<uint32_t, uint32_t> entry = {{ack_msg -> proposer, ack_msg -> proposed_seq}};
//                    this -> proposals[ack_msg -> msg_id] = entry;
//                } else {
                this -> proposals[ack_msg -> msg_id][ack_msg -> proposer] = ack_msg -> proposed_seq;
//                }
                if (this -> proposals.find(ack_msg -> msg_id) -> second.size() == this -> num_of_nodes - 1) {
                    SeqMessage* seq_msg = generate_seq_msg(ack_msg);
                    broadcast_final_seq(seq_msg);
                    logger -> info("all ack has been received, final seq sent");
                }
            }
            break;
        }
        case msg_type::seq:
        {
            SeqMessage* seq_msg = ntoh((SeqMessage *) buffer);
            handle_seq_msg(seq_msg);
            break;
        }
        default:
            logger -> error("unknow type message");
            break;
    }
}
void ISIS::handle_seq_msg(SeqMessage* seq_msg) {
    const auto logger = spdlog::get("console");
    logger -> info("received seq msg: id {}, sender: {}, final seq: {}, final proposer: {}",
                   seq_msg ->msg_id, seq_msg ->sender, seq_msg ->final_seq, seq_msg ->final_seq_proposer);
    this -> curr_seq = std::max(this -> curr_seq, seq_msg -> final_seq);
    CachedMsg* msg_to_be_changed = find_msg(seq_msg -> msg_id, seq_msg -> sender);
    if (msg_to_be_changed == nullptr) {
        logger -> error("unable to find the message");
    } else {
        msg_to_be_changed -> sequence_num = seq_msg -> final_seq;
        msg_to_be_changed -> proposer = seq_msg -> final_seq_proposer;
        msg_to_be_changed -> deliverable = true;

        logger -> info("changing element upon final seq, msg is id: {}, seq num: {}, proposer: {}",
                       msg_to_be_changed ->message_id, msg_to_be_changed ->sequence_num, msg_to_be_changed->proposer);
        this -> handle_q_change();
    }
}
bool ISIS::ack_has_received(AckMessage *msg) {
    auto msg_id = msg -> msg_id;
    if (this -> proposals.count(msg_id) == 0) return false;
    auto entry = this -> proposals.find(msg_id);
    if (entry -> second.count(msg -> proposer) == 0) return false;
    return true;
}
CachedMsg* ISIS::find_msg(uint32_t msg_id, uint32_t sender_id) {
    const auto logger = spdlog::get("console");
    CachedMsg* target;
    for (auto & n : this -> msg_q) {
        uint32_t id = n.message_id;
        uint32_t sender = n.sender_id;

        if (id == msg_id && sender_id == sender) {
            target = &n;
        }
    }
    if (target == nullptr) {
        logger -> error("unable to find the message");
    }
    return target;
}

void ISIS::broadcast_final_seq(SeqMessage* msg){
    const auto logger = spdlog::get("console");
    logger -> info ("broadcasting seq msg, id: {}, sender: {}, final seq: {}, final proposer: {}"
            , msg ->msg_id, msg ->sender, msg ->final_seq, msg ->final_seq_proposer);
    char buffer[sizeof(SeqMessage)];
    memcpy(buffer, msg, sizeof(SeqMessage));

//    std::cout << this -> my_id << ": "
//              << "Processed message " << msg -> msg_id << " from sender "
//              << msg -> sender << " with seq " << msg -> final_seq << ", "
//              << msg -> final_seq_proposer << std::endl;
    handle_seq_msg((SeqMessage *)buffer);

    this -> isblocked = false;
    SeqMessage* seq_msg = hton(msg);
    if (seq_msg != nullptr) {
        for (uint32_t id = 0; id < this -> num_of_nodes; id ++) {
            if (id == this -> my_id) continue;
            send_msg(seq_msg, addr_book[id], sizeof(SeqMessage));
        }
    }
}
SeqMessage* ISIS::generate_seq_msg(AckMessage* ack_msg) {
    const auto logger = spdlog::get("console");
//    for (const auto &n : this -> proposals) {
//        logger -> critical("message id {}", n.first);
//        for (const auto &m : n.second) {
//            logger -> critical("proposer is: {}, proposed seq: {}", m.first, m.second);
//        }
//    }
    auto msg_id = ack_msg -> msg_id;
    auto entry = this -> proposals.find(msg_id);
    int max_seq = -1;
    int max_proposer = -1;

    for (const auto &n : entry -> second) {
        auto proposer_id = n.first;
        auto proposer_seq = n.second;
//        logger -> critical("proposer_id: {}, proposed seq: {}", proposer_id, proposer_seq);
        // second: proposer, seq
        if (proposer_seq == max_seq && proposer_id < max_proposer) {
            max_seq = proposer_seq;
            max_proposer = proposer_id;
        } else if (proposer_seq > max_seq) {
            max_seq = proposer_seq;
            max_proposer = proposer_id;
        }
    }
    auto * msg = new SeqMessage;
    msg -> type = 3;
    msg -> sender = ack_msg -> sender;
    msg -> msg_id = ack_msg -> msg_id;
    msg -> final_seq = max_seq;
    msg -> final_seq_proposer = max_proposer;
//    logger -> critical("proposed msg: final seq {}, final prop: {}", max_seq, max_proposer);
    return msg;
}
AckMessage* ISIS::generate_ack_msg(DataMessage *msg) {
    AckMessage* ack = new AckMessage;
    ack -> type = 2;
    ack -> sender = msg -> sender;
    ack -> msg_id = msg -> msg_id;
    ack -> proposed_seq = this -> curr_seq;
    ack -> proposer = this -> my_id;

    return ack;
}
void ISIS::send_ack_msg(DataMessage *msg) {
    const auto &logger = spdlog::get("console");
    AckMessage * ack = generate_ack_msg(msg);

    logger -> info("sending ack msg id: {}, sender: {}, proposed seq: {}, proposer: {}",
            ack ->msg_id, ack ->sender, ack->proposed_seq, ack->proposer);
    hton(ack);

    if (ack != nullptr) {
        send_msg(ack, addr_book[msg -> sender], sizeof(AckMessage));
        delete(ack);
    }
}
void ISIS::enque_msg(DataMessage *msg) {
    const auto logger = spdlog::get("console");
    CachedMsg *cache_msg = new CachedMsg;
    cache_msg -> data = msg -> data;
    cache_msg -> message_id = msg -> msg_id;
    cache_msg -> sender_id = msg -> sender;
    cache_msg -> sequence_num = this -> curr_seq;
    cache_msg -> proposer = this -> my_id;
    cache_msg -> deliverable = false;

    this -> msg_q.push_back(*cache_msg);
    logger -> info("pushed message into the q: data: {}, id: {}, sender: {}, seq_num: {}, proposer: {}, deliverable: {} ",
            cache_msg -> data, cache_msg -> message_id, cache_msg -> sender_id, cache_msg -> sequence_num,
            cache_msg -> proposer, cache_msg -> deliverable);
    this -> handle_q_change();
}
void ISIS::handle_q_change() {
    const auto logger = spdlog::get("console");
    logger -> info("handling queue: size: {}", this -> msg_q.size());
    for (const auto &msg : this -> msg_q) {
        logger -> info("message is id: {}, sender: {}, proposer: {}, deliverable: {}. seqnum: {}",
                msg.message_id, msg.sender_id, msg.proposer, msg.deliverable, msg.sequence_num);
    }

    std::sort(
            this -> msg_q.begin(),
            this -> msg_q.end(),
            [] (const CachedMsg& a, const CachedMsg& b) {
                if (a.sequence_num != b.sequence_num) {
                    return a.sequence_num < b.sequence_num;
                } else if (a.deliverable == b.deliverable) {
                    return a.proposer < b.proposer;
                } else {
                    return !a.deliverable;
                }
            });

    while (!this -> msg_q.empty() && this -> msg_q.front().deliverable) {
        CachedMsg first_msg = msg_q.front();
        deliver_msg(&first_msg);
        this -> msg_q.erase(this -> msg_q.begin());
    }
}
void ISIS::deliver_msg(CachedMsg *msg) {
    const auto logger = spdlog::get("console");
    logger -> critical("start delivering msg");
    std::cout << this -> my_id << ": " << "Processed message " << msg -> message_id << " from sender "
    << msg -> sender_id << " with seq " << msg -> sequence_num << ", "
    << msg -> proposer << std::endl;
    this -> isblocked = false;
}
bool ISIS::has_duplication(DataMessage *msg) {
    if (this->past_msgs.empty()) return false;
    int curr_sender_id = msg -> sender;
    int curr_msg_id = msg -> msg_id;
    for (const auto &n : this -> past_msgs) {
        int sender_id = std::get<0>(n);
        int msg_id = std::get<1>(n);

        if (sender_id == curr_sender_id && msg_id == curr_msg_id) {
            return true;
        }
    }
    this -> past_msgs.emplace_back(curr_sender_id, curr_msg_id);
    return false;
}
msg_type ISIS::check_msg_type(void *msg, ssize_t size) {
    const auto logger = spdlog::get("console");
    uint32_t *first_int = (uint32_t *) msg;
    *first_int = ntohl(*first_int);

    if (size == sizeof(DataMessage) && *first_int == 1) {
        return msg_type::data;
    } else if (size == sizeof(AckMessage) && *first_int == 2) {
        return msg_type::ack;
    } else if (size == sizeof(SeqMessage) && *first_int == 3) {
        return msg_type::seq;
    } else {
        logger -> error("unable to identify incoming message");
        return msg_type::unknown;
    }
}
void ISIS::establish_connection() {
    // more formal connection scheme needed, for now we just wait
    if (this -> msg_count == 0) {
        this -> curr_state = state::receiving_msg;
    } else {
        const auto logger = spdlog::get("console");
        for (int i = 0; i < 5; i ++) {
            sleep(1);
            logger -> info("waiting for establishing connection, {}", i);
        }
    }
}
void ISIS::assess_next_state() {
    const auto logger = spdlog::get("console");

    switch (this -> curr_state) {
        case establishing_connection:
        {
            if (this -> counter == this -> msg_count) {
                this -> curr_state = state::receiving_msg;
            } else if (this -> isblocked) {
                this -> curr_state = state::receiving_msg;
            } else {
                this -> curr_state = state::sending_data_msg;
                this -> start_time = std::chrono::steady_clock::now();
                logger -> info("transition to state {}", this -> curr_state);
            }
            break;
        }
        case sending_data_msg:
        {
            if (this -> isblocked && calc_elapsed_time() < TIME_OUT) {
                this -> curr_state = state::receiving_msg;
                this -> start_time = std::chrono::steady_clock::now();
            } else if (this -> isblocked && calc_elapsed_time() > TIME_OUT) {
                broadcast_msg_to_timeout_nodes();
            } else if (!this -> isblocked && this -> counter < this -> msg_count) {
                this -> curr_state = state::sending_data_msg;
            } else if (!this -> isblocked && this -> counter >= this -> msg_count) {
                this -> curr_state = state::receiving_msg;
            }
            break;
        }
        case receiving_msg:
        {
            if (this -> counter == this -> msg_count) {
                this -> curr_state = state::receiving_msg;
            } else if (this -> isblocked && calc_elapsed_time() < TIME_OUT) {
                this -> curr_state = state::receiving_msg;
            } else if (this -> isblocked && calc_elapsed_time() > TIME_OUT) {
                broadcast_msg_to_timeout_nodes();
                this -> curr_state = state::receiving_msg;
            } else {
                this -> curr_state = state::sending_data_msg;
                logger -> info("transition to state {}", this -> curr_state);
            }

            break;
        }
        default:
            logger -> error("unknown state");
            break;
    }
}
void ISIS::broadcast_msg_to_timeout_nodes() {
    const auto logger = spdlog::get("console");
    logger -> info("resending msg");
    DataMessage * msg = generate_data_msg();
    if (msg == nullptr) return;

    for (uint32_t id = 0; id < this -> num_of_nodes; id++) {
        if (id != this -> my_id || this -> proposals.find(msg->msg_id) -> second.count(id) == 0)
        send_msg(msg, addr_book[id], sizeof(DataMessage));
    }
    delete(msg);
}
long long int ISIS::calc_elapsed_time() {
     return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now() - this -> start_time).count();
}
void ISIS::run_isis() {
    const auto logger = spdlog::get("console");
    while (true) {
        switch (curr_state) {
            case establishing_connection:
                establish_connection();
                assess_next_state();
                break;
            case sending_data_msg:
                broadcast_data_msg();
                assess_next_state();
                break;
            case receiving_msg:
                recv_msg();
                assess_next_state();
                break;
            default:
                logger -> error("unknown state");
                break;
        }
    }
}
