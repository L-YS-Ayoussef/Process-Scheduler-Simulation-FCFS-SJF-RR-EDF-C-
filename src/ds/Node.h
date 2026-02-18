#pragma once

template <typename T>
struct Node
{
    T data;
    Node *next;

    Node(const T &d) : data(d), next(nullptr) {}
};
