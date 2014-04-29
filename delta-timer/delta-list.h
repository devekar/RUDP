#ifndef _DELTA_LIST_H_
#define _DELTA_LIST_H_

#include "types.h"
#include <stdio.h>

template <typename Data_T>
class DeltaList
{

protected:
    struct Node_T
    {
        UINT32 priority;
        Data_T data;
        Node_T *next;
    };
    Node_T *_head;
    UINT32 _count;

public:
    DeltaList();
    //For timer, priority is the number of seconds after which the timer should fire
    Error_T Enqueue(UINT32 priority, Data_T data);
    Error_T Dequeue(UINT32 &priority, Data_T& data);
    Error_T Peek(UINT32 &priority, Data_T& data);
    Error_T SetHeadPriority(UINT32 priority);
    UINT32 Count();
    void Print();
};

#define nullptr 0

template <typename Data_T>
DeltaList<Data_T>::DeltaList()
{
    _head = nullptr;
    _count = 0;
}

template <typename Data_T>
Error_T DeltaList<Data_T>::Enqueue(UINT32 priority, Data_T data)
{
    if (priority < 0 || priority > 0xFFFFFFFF)
        return 1;

    Node_T *node = new Node_T();
    node->next = nullptr;
    node->priority = priority;
    node->data = data;

    Node_T* prevNode = nullptr;
    Node_T* currNode = _head;
    while(currNode != nullptr && currNode->priority <= node->priority)
    {
        node->priority -= currNode->priority;
        prevNode = currNode;
        currNode = currNode->next;
    }

    if (currNode == _head)
        _head = node;

    if (prevNode != nullptr)
        prevNode->next = node;

    node->next = currNode;
    if (currNode != nullptr)
        currNode->priority -= node->priority;

    _count++;
    return 0;
}

template <typename Data_T>
Error_T DeltaList<Data_T>::Dequeue(UINT32 &priority, Data_T &data)
{
    if (_count == 0 || _head == nullptr)
        return 1;

    priority = _head->priority;
    data = _head->data;

    Node_T* temp = _head;
    _head = _head->next;

    delete temp;
    _count--;
    return 0;
}

template <typename Data_T>
Error_T DeltaList<Data_T>::Peek(UINT32 &priority, Data_T &data)
{
    if (_count == 0 || _head == nullptr)
            return 1;

    priority = _head->priority;
    data = _head->data;

    return 0;
}

template <typename Data_T>
UINT32 DeltaList<Data_T>::Count()
{
    return _count;
}

template <typename Data_T>
Error_T DeltaList<Data_T>::SetHeadPriority(UINT32 priority)
{
    _head->priority = priority;
}

template <typename Data_T>
void DeltaList<Data_T>::Print()
{
    if (_count == 0 && _head == nullptr)
    {
        puts("List is empty");
        return;
    }

    puts("Printing list:");

    Node_T* node = _head;
    int i = 1;

    while (node != nullptr)
    {
        printf("Node %d: Priority: %d\n", i++, node->priority);
        node = node->next;
    }
}

#endif
