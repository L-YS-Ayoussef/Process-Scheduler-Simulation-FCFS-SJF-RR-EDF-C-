#include "processors/FCFSProcessor.h"
#include "model/Process.h"

void FCFSProcessor::enqueue(Process *p)
{
    rdy.enqueue(p);
    readyWork += p->getRemaining();
}

Process *FCFSProcessor::popReady()
{
    Process *p = nullptr;
    if (!rdy.dequeue(p))
        return nullptr;
    readyWork -= p->getRemaining();
    return p;
}

Process *FCFSProcessor::peekReady() const
{
    Node<Process *> *node = rdy.getHead();
    return node ? node->data : nullptr;
}

void FCFSProcessor::printReady(std::ostream &os) const
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

bool FCFSProcessor::removeReadyByPID(int pid, Process *&out)
{
    Queue<Process *> temp;
    bool found = false;

    while (!rdy.empty())
    {
        Process *p = nullptr;
        rdy.dequeue(p);

        if (!found && p && p->getPID() == pid)
        {
            out = p;
            found = true;
            readyWork -= p->getRemaining();
        }
        else
        {
            temp.enqueue(p);
        }
    }

    while (!temp.empty())
    {
        Process *p = nullptr;
        temp.dequeue(p);
        rdy.enqueue(p);
    }

    return found;
}
