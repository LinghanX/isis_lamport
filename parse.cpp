//
// Created by Linghan Xing on 10/1/18.
//

#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fstream>
#include <iostream>

std::vector<std::string> parsePath(std::string paths) {
    std::vector<std::string> hostAdds;
    std::ifstream file(paths);
    std::string str;
    while (std::getline(file, str))
    {
        hostAdds.push_back(str);
    }
    return hostAdds;
}

std::tuple<std::vector<std::string>, int, int> handle_input(int argc, char **argv)
{
    auto logger = spdlog::get("console");

    int option;
    int pflag = 0;
    int hflag = 0;
    int cflag = 0;
    int portNum = 0;
    char *path;
    int count = 0;
    std::vector<std::string> neighbors;

    while ((option = getopt(argc, argv, "p:h:c:")) != -1) {
        switch (option) {
            case 'p':
                portNum = atoi(optarg);
                pflag = 1;
                break;
            case 'h':
                path = optarg;
                neighbors = parsePath(path);
                hflag = 1;
                break;
            case 'c':
                count = atoi(optarg);
                cflag = 1;
                break;
            case '?':
                if (optopt == 'p')
                    fprintf(stderr, "Option -%c needs argument\n", optopt);
                else fprintf(stderr, "Unknown option -%c. \n", optopt);
                break;
            default:
                fprintf(stderr, "error");
        }
    }


    if (cflag == 0 || hflag == 0 || pflag == 0) {
        logger -> error("please input all required options");
        exit(1);
    }

    if (portNum < 1024 ||  portNum > 65535) {
        logger -> error("PortNumber out of range", portNum);
        exit(1);
    }

    logger -> info("finished processing input");
    logger -> info("the selected portal is: {}", portNum);
    logger -> info("the selected count is: {}", count);
    logger -> info("the selected file path is: {}", path);

    return std::make_tuple(neighbors, portNum, count);
}