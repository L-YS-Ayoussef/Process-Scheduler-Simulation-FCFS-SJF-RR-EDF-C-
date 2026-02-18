#pragma once
#include "processors/Processor.h"
#include "ds/MinHeap.h"
#include <ostream>

class Process;

class EDFProcessor : public Processor
{
private:
    static bool lessEDF(Process *const &a, Process *const &b);
    MinHeap<Process *> heap;

public:
    EDFProcessor(int id) : Processor(id, ProcType::EDF), heap(&EDFProcessor::lessEDF) {}

    void enqueue(Process *p) override;
    Process *popReady() override;
    Process *peekReady() const override;
    std::size_t readyCount() const override { return heap.size(); }
    void printReady(std::ostream &os) const override;
};
