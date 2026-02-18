#pragma once
#include "processors/Processor.h"
#include "ds/Queue.h"
#include <ostream>

class Process;

class FCFSProcessor : public Processor
{
private:
    Queue<Process *> rdy;

public:
    FCFSProcessor(int id) : Processor(id, ProcType::FCFS) {}

    void enqueue(Process *p) override;
    Process *popReady() override;
    Process *peekReady() const override;
    std::size_t readyCount() const override { return rdy.size(); }
    void printReady(std::ostream &os) const override;

    bool removeReadyByPID(int pid, Process *&out);
};
