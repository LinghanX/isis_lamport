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

    std::vector<std::string> addr_book;
    std::string port;
    int count;

    std::tie(addr_book, port, count) = handle_input(argc, argv);
    ISIS *program = new ISIS(addr_book, port, count);
    program -> start();

    exit(0);
}