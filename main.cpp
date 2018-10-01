#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include "messages.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include "parse.h"
#include "isis.h"

/*
 * assumptions:
 *     1) all processes operates on the same port
 *     2) host file includes my name
 */

int main (int argc, char **argv) {
    auto console = spdlog::stdout_color_mt("console");
    console->info("welcome, starting program!") ;

    std::vector<std::string> host_names;
    std::unordered_map<int, std::string> id_to_host_name;
    int port;
    int count;

    std::tie(host_names, port, count) = handle_input(argc, argv);
    ISIS *program = new ISIS(id_to_host_name, port, count);
    program -> broadcast();

    exit(0);
}