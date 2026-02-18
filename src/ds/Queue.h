#pragma once
#include "LinkedList.h"

template <typename T>
class Queue
{
private:
    LinkedList<T> list;

public:
    Queue() = default;

    bool empty() const { return list.empty(); }
    std::size_t size() const { return list.size(); }

    void enqueue(const T &value) { list.pushBack(value); }
    bool dequeue(T &out) { return list.popFront(out); }

    Node<T> *getHead() const { return list.getHead(); } // for printing
};
