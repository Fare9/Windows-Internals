#pragma once

#include "headers.h"
#include "Thread.h"

#ifndef PROCESS_H
#define PROCESS_H


class Process
{
public:

    Process(
        ULONG process_id,
        ULONG parent_process_id,
        std::wstring  image_file_name,
        std::wstring command,
        ItemType state,
        SYSTEMTIME st);

    Process(ULONG process_id);

    ~Process();

    void                ChangeProcessState(ItemType state);
    void                InserThread(Thread thread);
    void                RemoveThread(ULONG thread_id);

    friend bool operator== (const Process &n1, const Process &n2);

    // Getters
    const ULONG         ProcessID();
    const ULONG         ParentprocessID();
    const std::wstring  ImageFileName();
    const std::wstring  Command();
    const ItemType      State();
    const SYSTEMTIME    ST();
    const std::vector<Thread> ThreadList();

private:
    ULONG                   process_id_;
    ULONG                   parent_process_id_;
    std::wstring            image_file_name_;
    std::wstring            command_;
    ItemType                state_;
    SYSTEMTIME              st_;
    std::vector<Thread>      thread_list_;
};


#endif // !PROCESS_H
