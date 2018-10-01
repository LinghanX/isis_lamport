//
// Created by Linghan Xing on 10/1/18.
//

#ifndef ISIS_LAMPORT_ISIS_H
#define ISIS_LAMPORT_ISIS_H
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

class ISIS {
public:
    ISIS(const std::unordered_map<std::string, int>&, int, int);
    void broadcast();
};

#endif //ISIS_LAMPORT_ISIS_H
