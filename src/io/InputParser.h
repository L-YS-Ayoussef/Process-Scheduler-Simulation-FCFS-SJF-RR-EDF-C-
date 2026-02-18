#pragma once
#include <string>
#include "ds/LinkedList.h"
#include "model/Process.h"
#include "model/KillEvent.h"

struct ParsedInput
{
    int NF{}, NS{}, NR{};
    int timeSlice{};
    int RTF{}, MaxW{}, STL{}, forkProb{};
    int M{};

    LinkedList<Process *> newList;      // NEW processes (sorted by AT in input)
    LinkedList<KillEvent> killEvents;   // (time, pid) pairs
    LinkedList<Process *> allProcesses; // for memory ownership cleanup
};

class InputParser
{
public:
    static bool parseFile(const std::string &path, ParsedInput &out, std::string &err);
};
