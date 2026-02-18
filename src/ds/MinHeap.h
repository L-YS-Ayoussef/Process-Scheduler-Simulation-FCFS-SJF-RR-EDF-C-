#pragma once
#include <cstddef>
#include <stdexcept>

template <typename T>
class MinHeap
{
public:
    using LessFunc = bool (*)(const T &, const T &);

private:
    T *arr;
    std::size_t cap;
    std::size_t n;
    LessFunc less;

    void swap(T &a, T &b)
    {
        T tmp = a;
        a = b;
        b = tmp;
    }

    void ensureCap()
    {
        if (n < cap)
            return;
        std::size_t newCap = (cap == 0) ? 8 : cap * 2;
        T *newArr = new T[newCap];
        for (std::size_t i = 0; i < n; ++i)
            newArr[i] = arr[i];
        delete[] arr;
        arr = newArr;
        cap = newCap;
    }

    void heapifyUp(std::size_t idx)
    {
        while (idx > 0)
        {
            std::size_t parent = (idx - 1) / 2;
            if (!less(arr[idx], arr[parent]))
                break;
            swap(arr[idx], arr[parent]);
            idx = parent;
        }
    }

    void heapifyDown(std::size_t idx)
    {
        while (true)
        {
            std::size_t left = idx * 2 + 1;
            std::size_t right = idx * 2 + 2;
            std::size_t smallest = idx;

            if (left < n && less(arr[left], arr[smallest]))
                smallest = left;
            if (right < n && less(arr[right], arr[smallest]))
                smallest = right;

            if (smallest == idx)
                break;
            swap(arr[idx], arr[smallest]);
            idx = smallest;
        }
    }

public:
    MinHeap(LessFunc lf) : arr(nullptr), cap(0), n(0), less(lf) {}
    ~MinHeap() { delete[] arr; }

    MinHeap(const MinHeap &) = delete;
    MinHeap &operator=(const MinHeap &) = delete;

    bool empty() const { return n == 0; }
    std::size_t size() const { return n; }

    void push(const T &value)
    {
        ensureCap();
        arr[n] = value;
        heapifyUp(n);
        ++n;
    }

    T pop()
    {
        if (n == 0)
            throw std::runtime_error("Heap empty");
        T root = arr[0];
        --n;
        if (n > 0)
        {
            arr[0] = arr[n];
            heapifyDown(0);
        }
        return root;
    }

    const T *raw() const { return arr; }
    std::size_t rawSize() const { return n; }

    T peek() const
    {
        if (n == 0)
            return T{};
        return arr[0];
    }
};
