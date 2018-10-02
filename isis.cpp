//
// Created by Linghan Xing on 10/1/18.
//
#include "isis.h"

int get_my_id(char *name, const std::unordered_map<std::string, int> &map) {
    int id;
    for ( const auto& n : map ) {
        if (n.first.compare(name) == 0) {
            id = n.second;
        }
    }
    return id;
};
void ISIS::init() {
    auto logger = spdlog::get("console");
    struct addrinfo hints, *res, *res0;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // to allow unstable/reliable connection;
    hints.ai_flags = AI_PASSIVE;

    if ( (getaddrinfo(nullptr, port.c_str(), &hints, &res)) < 0) {
        logger -> error("not able to start listenning");
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

        // needs to use :: due to namespace
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

    logger -> info("start listenning on port: {}, with fd: {}", port, fd);
}

ISIS::~ISIS() {
    close( this-> listening_fd);
}

ISIS::ISIS( const std::unordered_map<std::string, int> &hostname_to_id,
            std::string port,
            int msg_num)
{
    auto logger = spdlog::get("console");
    logger -> info("initiating ISIS");
    char currHostName[MAX_HOSTNAME_LEN]; // my name
    if (gethostname(currHostName, MAX_HOSTNAME_LEN) < 0) {
        logger -> error("not able to get my host name");
    }

    this -> my_id = get_my_id(currHostName, hostname_to_id);

    if (my_id == 0) {
        logger -> error("unable to parse my id");
    }

    this -> port = port;
    this -> msg_count = msg_num;
    this -> hostname_to_id = hostname_to_id;

    init();
}

void ISIS::broadcast() {
}



