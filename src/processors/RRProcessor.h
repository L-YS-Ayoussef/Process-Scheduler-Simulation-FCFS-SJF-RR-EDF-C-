#pragma once
#include "processors/Processor.h"
#include "ds/Queue.h"
#include <ostream>

class Process;

class RRProcessor : public Processor
{
private:
    Queue<Process *> rdy;

public:
    RRProcessor(int id) : Processor(id, ProcType::RR) {}

    void enqueue(Process *p) override;
    Process *popReady() override;
    Process *peekReady() const override; 
    std::size_t readyCount() const override { return rdy.size(); }
    void printReady(std::ostream &os) const override;
};
