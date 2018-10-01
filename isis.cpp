//
// Created by Linghan Xing on 10/1/18.
//
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include "isis.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <iostream>

using namespace std;

int get_my_id(char *name, const std::unordered_map<std::string, int> &map) {
    int id;
    for ( const auto& n : map ) {
        if (n.first.compare(name) == 0) {
            id = n.second;
        }
    }
    return id;
};

ISIS::ISIS(const unordered_map<string, int> &hostname_to_id, int port, int msg_num) {
    auto logger = spdlog::get("console");
    logger -> info("initiating ISIS");

    char currHostName[MAX_HOSTNAME_LEN]; // my name

    if (gethostname(currHostName, MAX_HOSTNAME_LEN) < 0) {
        logger -> error("not able to get my host name");
    }

    my_id = get_my_id(currHostName, hostname_to_id);
//    cout << "my name is: " << currHostName << endl;
//    cout << "my id is: " << my_id << endl;
}

void ISIS::broadcast() {
}



