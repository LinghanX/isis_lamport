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
    logger -> info("initiating ISIS");
    char currHostName[MAX_HOSTNAME_LEN]; // my name
    if (gethostname(currHostName, MAX_HOSTNAME_LEN) < 0) {
        logger -> error("not able to get my host name");
    }

    int id = 0;
    bool find_id = false;
    for (auto const& addr : addr_book) {
        if (addr.compare(currHostName) == 0) {
            find_id = true;
            this -> my_id = id;
            break;
        } else id++;
    }

    if (!find_id)
        logger -> error("unable to parse my id");

    this -> port = port;
    this -> msg_count = msg_num;
    this -> addr_book = addr_book;
    this -> curr_state = sending_data_msg;
    this -> num_of_nodes = static_cast<int>(addr_book.size());

    for (int i = 0; i < num_of_nodes; i++) {
        this -> seq.push_back(0);
        this -> ack.push_back(false);
    }

    start();
}

DataMessage* ISIS::generate_data_msg() {
    auto *msg = new DataMessage;
    msg -> type = 1;
    msg -> sender = this -> my_id;
    msg -> data = DUMMY_DATA;
    msg -> msg_id = static_cast<uint32_t>(this -> msg_sent); // the msg_id is always num of msg sent

    return msg;
}

void ISIS::broadcast_data_msg() {
    struct timeval start;
    uint32_t elapsed_time = 0;
    const auto logger = spdlog::get("console");
    logger -> info("start broadcasting");
    DataMessage *msg = generate_data_msg();
    int num_of_msg_send = 0;
    std::vector<bool> msg_sent(static_cast<unsigned long>(this -> num_of_nodes), false);

    if (msg == nullptr)
        logger -> error("unable to allocate data message");
    this -> increment_seq();
    // before we send, we need to convert message to network endian
    auto * converted_msg = hton(msg);

    // seq.size() is the total num of id;
    while (num_of_msg_send < this -> num_of_nodes - 1 && elapsed_time < TIME_OUT) {
        for (uint32_t id = 0; id < this -> seq.size(); id++) {
            // nothing to do when its myself, or the msg has been sent
            if (id == this -> my_id || msg_sent[id]) continue;
            bool sent =
                    this->send_msg(converted_msg, this->addr_book[id], sizeof(DataMessage));
            if (sent) {
                num_of_msg_send++;
                msg_sent[id] = true;
            }
            struct timeval end;
            gettimeofday(&end, nullptr);
            elapsed_time = static_cast<uint32_t>(
                    (end.tv_sec * 1000000 + end.tv_usec) -
                    ((start).tv_sec * 1000000 + (start).tv_usec));
        }
    }
    delete(msg);

    this -> msg_sent += 1;
    this -> curr_state = waiting_ack;
    logger -> info("all data message successfully sent");
}

bool ISIS::send_msg(void *msg, std::string addr, uint32_t size) {
    const auto logger = spdlog::get("console");
    logger -> info("start sending message to addr");

    int sock_fd, status, num_bytes;
    struct addrinfo hint, *res, *res0;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_PASSIVE;

    if ( (status = getaddrinfo(addr.c_str(), this -> port.c_str(), &hint, &res)) < 0 ) {
        logger -> error("unable to get host address");
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
    logger -> info("successfully sent message {}", msg);

    close(sock_fd);
    freeaddrinfo(res);

    return true;
}
void ISIS::increment_seq() {
    this -> seq[this -> my_id] += 1;
}

void ISIS::recv_msg() {
    const auto logger = spdlog::get("console");
    logger -> info("start receiving message");
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
        logger -> error("unable to receive message");
    }

    msg_type type = check_msg_type(buffer, num_bytes);
    switch (type) {
        case msg_type::data:
        {
            DataMessage* msg = ntoh((DataMessage *) buffer);
            if(!has_duplication(msg)) {
                this -> seq[this -> my_id] += 1;
                send_ack_msg(msg);
                enque_msg(msg);
            }
            break;
        }
    }
}

AckMessage* ISIS::generate_ack_msg(DataMessage *msg) {
    AckMessage* ack = new AckMessage;
    ack -> type = 2;
    ack -> sender = msg -> sender;
    ack -> msg_id = msg -> msg_id;
    ack -> proposed_seq = static_cast<uint32_t>(this -> seq[this -> my_id]);
    ack -> proposer = this -> my_id;

    return ack;
}

void ISIS::send_ack_msg(DataMessage *msg) {
    AckMessage * ack = generate_ack_msg(msg);
    hton(ack);

    if (ack != nullptr) {
        send_msg(ack, addr_book[msg -> sender], sizeof(AckMessage));
        delete(ack);
    }
}
void ISIS::enque_msg(DataMessage *msg) {
    CachedMsg *cache_msg = new CachedMsg;
    cache_msg -> data = msg -> data;
    cache_msg -> message_id = msg -> msg_id;
    cache_msg -> sender_id = msg -> sender;
    cache_msg -> sequence_num = static_cast<uint32_t>(this -> seq[this -> my_id]);
    cache_msg -> proposer = this -> my_id;
    cache_msg -> deliverable = false;

    this -> msg_q.push_back(*cache_msg);
    this -> handle_q_change();
}
void ISIS::handle_q_change() {
    std::sort(
            this -> msg_q.begin(),
            this -> msg_q.end(),
            [] (CachedMsg a, CachedMsg b) {
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
    std::cout << this -> my_id << ": "
    << "Processed message " << msg -> message_id << " from sender "
    << msg -> sender_id << " with seq " << msg -> sequence_num << ", "
    << msg -> proposer << std::endl;
    free(msg);
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
void ISIS::recv_seq() {}

void ISIS::start() {
    auto logger = spdlog::get("console");
    logger -> info("start algorithm");
    char buffer[BUFFER_SIZE];
    ssize_t byte_num;
    struct sockaddr_in neighbor;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct timeval start;
    struct timeval end;

    while (true) {
        switch (curr_state) {
            case sending_data_msg:
                broadcast_data_msg();
                break;
            case receiving_msg:
                recv_msg();
                break;
            case waiting_seq:
                recv_seq();
                break;
            default:
                logger -> error("unknown state");
                break;
        }
    }
}
