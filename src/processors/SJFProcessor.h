#pragma once
#include "processors/Processor.h"
#include "ds/MinHeap.h"
#include <ostream>

class Process;

class SJFProcessor : public Processor
{
private:
    static bool lessProc(Process *const &a, Process *const &b);
    MinHeap<Process *> heap;

public:
    SJFProcessor(int id) : Processor(id, ProcType::SJF), heap(&SJFProcessor::lessProc) {}

    void enqueue(Process *p) override;
    Process *popReady() override;
    Process *peekReady() const override; // NEW
    std::size_t readyCount() const override { return heap.size(); }
    void printReady(std::ostream &os) const override;
};
