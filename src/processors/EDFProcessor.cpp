#include "processors/EDFProcessor.h"
#include "model/Process.h"

bool EDFProcessor::lessEDF(Process *const &a, Process *const &b)
{
    int da = a->hasDeadline() ? a->getDeadline() : INT_MAX;
    int db = b->hasDeadline() ? b->getDeadline() : INT_MAX;
    if (da != db)
        return da < db;
    return a->getPID() < b->getPID();
}

void EDFProcessor::enqueue(Process *p)
{
    heap.push(p);
    readyWork += p->getRemaining();
}

Process *EDFProcessor::popReady()
{
    if (heap.empty())
        return nullptr;
    Process *p = heap.pop();
    readyWork -= p->getRemaining();
    return p;
}

Process *EDFProcessor::peekReady() const
{
    return heap.peek();
}

void EDFProcessor::printReady(std::ostream &os) const
{
    Process *const *raw = heap.raw();
    std::size_t n = heap.rawSize();
    for (std::size_t i = 0; i < n; ++i)
    {
        os << raw[i]->getPID();
        if (i + 1 < n)
            os << ",";
    }
}
