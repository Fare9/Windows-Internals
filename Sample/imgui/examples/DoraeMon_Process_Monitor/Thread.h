#pragma once

#include "headers.h"


#ifndef THREAD_H
#define THREAD_H


class Thread
{
public:
    Thread(ItemType state, ULONG thread_id, ULONG process_owner_id, SYSTEMTIME st);
    Thread(ULONG thread_id);
    ~Thread();

    const ItemType State();
    const ULONG ThreadID();
    const ULONG ProcessOwner();
    const SYSTEMTIME ST();

    friend bool operator== (const Thread &n1, const Thread &n2);
private:
    ItemType        state_;
    ULONG           thread_id_;
    SYSTEMTIME      st_;
    ULONG           process_owner_id_;
};

#endif // !THREAD_H
