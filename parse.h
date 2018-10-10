//
// Created by Linghan Xing on 10/1/18.
//

#ifndef ISIS_LAMPORT_PARSE_H
#define ISIS_LAMPORT_PARSE_H
#include <vector>
#include <unordered_map>
using namespace std;

vector<string> parsePath(string paths);
tuple<vector<string>, string, int, int> handle_input(int argc, char **argv);
#endif //ISIS_LAMPORT_PARSE_H
