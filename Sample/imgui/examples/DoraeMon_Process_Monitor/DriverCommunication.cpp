#include "DriverCommunication.h"
#include "GUI_Processes_Functions.h"

// Constant values
#define BUFFER_SIZE 16 // 64KB


// Variables
HANDLE hFile;
BYTE buffer[1 << BUFFER_SIZE] = { 0 };
DWORD bytes;

std::vector<Process> processes;
std::vector<Thread> threads;

// private function
BOOL Error(const char* msg)
{
    printf("%s: error=%d\n", msg, GetLastError());
    return FALSE;
}


BOOL OpenCommunicationWithDriver(const WCHAR* driverPath)
{
    hFile = CreateFile(driverPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return Error("Failed to open driver");
    }

    return TRUE;
}


BOOL ReadDataFromDriver()
{
    if (!ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr))
    {
        return Error("Failed to read");
    }

    return TRUE;
}


BOOL Monitorize()
{
    if (!ReadDataFromDriver())
    {
        return FALSE;
    }

    if (bytes)
    {
        auto count = bytes;
        auto local_buffer = buffer;

        while (count > 0)
        {
            auto header = (ItemHeader*)local_buffer;
            SaveStructure(local_buffer, header);
            local_buffer += header->size;
            count -= header->size;
        }
    }

    memset(buffer, 0, sizeof(buffer));

    return TRUE;
}


VOID SaveStructure(BYTE* buff, ItemHeader* item)
{
    SYSTEMTIME systemtime;

    FileTimeToSystemTime((FILETIME*)&item->Time, &systemtime);

    systemtime.wHour += 2;

    switch (item->Type)
    {
    case ItemType::ProcessCreate:
    {
        auto processInfo = (ProcessCreateInfo*)item;

        // if process is already on vector
        if (std::find(processes.begin(), processes.end(), Process(processInfo->ProcessId)) != processes.end())
            break;



        std::wstring CommandLine((WCHAR*)(buff + processInfo->CommandLineOffset), processInfo->CommandLineLength);

        std::wstring ImageFileName((WCHAR*)(buff + processInfo->ImageFileNameOffset), processInfo->ImageFileNameLength);

        printf("\n");
        printf("\tProcess ID: %X(%d)\n", processInfo->ProcessId, processInfo->ProcessId);
        printf("\tParent PID: %X(%d)\n", processInfo->ParentProcessId, processInfo->ParentProcessId);
        printf("\tSize: %X(%d)\n", processInfo->size, processInfo->size);

        wprintf(L"\tIMAGEFILENAME: %-256ws\n\tCOMMANDLINE: %-256ws\n", ImageFileName.c_str(), CommandLine.c_str());
        printf("\n");

        Process p(processInfo->ProcessId, processInfo->ParentProcessId, ImageFileName, CommandLine, processInfo->Type, systemtime);

        for (size_t i = 0; i < threads.size(); i++)
        {
            if (threads[i].ProcessOwner() == p.ProcessID())
            {
                p.InserThread(threads[i]);
                threads.erase(threads.begin() + i);
                i = 0;
           }
        }


        processes.push_back(p);

        break;
    }
    case ItemType::ProcessExit:
    {
        auto processInfo = (ProcessExitInfo*)item;

        std::vector<Process>::iterator it = std::find(processes.begin(), processes.end(), Process(processInfo->ProcessId));

        printf("\n");
        printf("\tProcess ID: %X(%d)\n", processInfo->ProcessId, processInfo->ProcessId);
        printf("\tParent PID: %s(%s)\n", "None", "None");
        printf("\tSize: %X(%d)\n", processInfo->size, processInfo->size);
        printf("\n");

        // if process is on vector, remove it
        if (it != processes.end())
        {
            processes.erase(it);
        }

        break;
    }
    case ItemType::ThreadCreate:
    {
        auto threadinfo = (ThreadCreateExitInfo*)buffer;

        Thread t(threadinfo->Type, threadinfo->ThreadId, threadinfo->ProcessId, systemtime);

        printf("\n");
        printf("\tThread ID: %X(%d)\n", threadinfo->ThreadId, threadinfo->ThreadId);
        printf("\tProcess ID: %X(%d)\n", threadinfo->ProcessId, threadinfo->ProcessId);
        printf("\tSize: %X(%d)\n", threadinfo->size, threadinfo->size);
        printf("\n");

        std::vector<Process>::iterator it_process = std::find(processes.begin(), processes.end(), Process(threadinfo->ProcessId));

        // if the thread is not in any process of list
        if (it_process == processes.end())
        {
            threads.push_back(t);
            break;
        }

        auto thread_vector = it_process->ThreadList();

        std::vector<Thread>::iterator it_thread = std::find(
            thread_vector.begin(),
            thread_vector.end(),
            Thread(threadinfo->ThreadId));

        // if thread does not exist on process, insert it
        if (it_thread == thread_vector.end())
        {
            it_process->InserThread(t);
        }

        break;
    }
    case ItemType::ThreadExit:
    {
        auto threadinfo = (ThreadCreateExitInfo*)buffer;

        printf("\n");
        printf("\tThread ID: %X(%d)\n", threadinfo->ThreadId, threadinfo->ThreadId);
        printf("\tProcess ID: %X(%d)\n", threadinfo->ProcessId, threadinfo->ProcessId);
        printf("\tSize: %X(%d)\n", threadinfo->size, threadinfo->size);
        printf("\n");

        std::vector<Process>::iterator it_process = std::find(processes.begin(), processes.end(), Process(threadinfo->ProcessId));

        if (it_process != processes.end())
        {
            it_process->RemoveThread(threadinfo->ThreadId);
        }

        std::vector<Thread>::iterator it_thread = std::find(threads.begin(), threads.end(), Thread(threadinfo->ThreadId));

        if (it_thread != threads.end())
        {
            threads.erase(it_thread);
        }

        break;
    }
    }
}


VOID PrintProcesses()
{
    if (processes.empty())
        return;

    for (auto process : processes)
    {
        ShowProcessOnList(process);
    }

    return;
}


BOOL CloseCommunicationWithDriver()
{
    if (!CloseHandle(hFile))
    {
        return Error("Error closing driver communication");
    }

    return TRUE;
}
