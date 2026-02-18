#include "processors/SJFProcessor.h"
#include "model/Process.h"

bool SJFProcessor::lessProc(Process *const &a, Process *const &b)
{
    if (a->getRemaining() != b->getRemaining())
        return a->getRemaining() < b->getRemaining();
    return a->getPID() < b->getPID();
}

void SJFProcessor::enqueue(Process *p)
{
    heap.push(p);
    readyWork += p->getRemaining();
}

Process *SJFProcessor::popReady()
{
    if (heap.empty())
        return nullptr;
    Process *p = heap.pop();
    readyWork -= p->getRemaining();
    return p;
}

Process *SJFProcessor::peekReady() const
{
    // You need a peek method in MinHeap:
    // If you don't have it, add it (shown below).
    return heap.peek();
}

void SJFProcessor::printReady(std::ostream &os) const
{
    // heap internal array order (not sorted), but fine for display
    Process *const *raw = heap.raw();
    std::size_t n = heap.rawSize();
    for (std::size_t i = 0; i < n; ++i)
    {
        os << raw[i]->getPID();
        if (i + 1 < n)
            os << ",";
    }
}
