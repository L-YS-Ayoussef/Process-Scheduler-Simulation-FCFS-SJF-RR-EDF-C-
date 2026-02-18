#include "processors/RRProcessor.h"
#include "model/Process.h"

void RRProcessor::enqueue(Process *p)
{
    rdy.enqueue(p);
    readyWork += p->getRemaining();
}

Process *RRProcessor::popReady()
{
    Process *p = nullptr;
    if (!rdy.dequeue(p))
        return nullptr;
    readyWork -= p->getRemaining();
    return p;
}

Process *RRProcessor::peekReady() const
{
    Node<Process *> *node = rdy.getHead();
    return node ? node->data : nullptr;
}

void RRProcessor::printReady(std::ostream &os) const
{
    Node<Process *> *cur = rdy.getHead();
    while (cur)
    {
        os << cur->data->getPID();
        if (cur->next)
            os << ",";
        cur = cur->next;
    }
}
