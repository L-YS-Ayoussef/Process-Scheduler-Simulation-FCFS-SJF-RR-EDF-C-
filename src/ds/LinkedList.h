#pragma once
#include "Node.h"
#include <cstddef>

template <typename T>
class LinkedList
{
private:
    Node<T> *head;
    Node<T> *tail;
    std::size_t count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    ~LinkedList() { clear(); }

    LinkedList(const LinkedList &) = delete;
    LinkedList &operator=(const LinkedList &) = delete;

    bool empty() const { return head == nullptr; }
    std::size_t size() const { return count; }

    Node<T> *getHead() const { return head; }

    void pushBack(const T &value)
    {
        Node<T> *n = new Node<T>(value);
        if (!tail)
            head = tail = n;
        else
        {
            tail->next = n;
            tail = n;
        }
        ++count;
    }

    void pushFront(const T &value)
    {
        Node<T> *n = new Node<T>(value);
        n->next = head;
        head = n;
        if (!tail)
            tail = head;
        ++count;
    }

    // pops front into out; returns false if empty
    bool popFront(T &out)
    {
        if (!head)
            return false;
        Node<T> *n = head;
        out = n->data;
        head = head->next;
        if (!head)
            tail = nullptr;
        delete n;
        --count;
        return true;
    }

    // remove first occurrence using operator==
    bool removeFirst(const T &value)
    {
        Node<T> *prev = nullptr;
        Node<T> *cur = head;
        while (cur)
        {
            if (cur->data == value)
            {
                if (prev)
                    prev->next = cur->next;
                else
                    head = cur->next;
                if (cur == tail)
                    tail = prev;
                delete cur;
                --count;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    void clear()
    {
        Node<T> *cur = head;
        while (cur)
        {
            Node<T> *nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        head = tail = nullptr;
        count = 0;
    }
};
