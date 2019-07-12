#include "Process.h"


Process::Process(ULONG process_id, ULONG parent_process_id, std::wstring  image_file_name, std::wstring command, ItemType state, SYSTEMTIME st) :
    process_id_(process_id),
    parent_process_id_(parent_process_id),
    image_file_name_(image_file_name),
    command_(command),
    state_(state),
    st_(st)
{}

Process::Process(ULONG process_id) :
    process_id_(process_id)
{}


Process::~Process()
{
    thread_list_.clear();
}


void Process::ChangeProcessState(ItemType state)
{
    this->state_ = state;
}


void Process::InserThread(Thread thread)
{
    this->thread_list_.push_back(thread);
}


const ULONG Process::ProcessID()
{
    return this->process_id_;
}


const ULONG Process::ParentprocessID()
{
    return this->parent_process_id_;
}


const std::wstring Process::ImageFileName()
{
    return this->image_file_name_;
}


const std::wstring Process::Command()
{
    return this->command_;
}


const ItemType Process::State()
{
    return this->state_;
}


const SYSTEMTIME Process::ST()
{
    return this->st_;
}


const std::vector<Thread> Process::ThreadList()
{
    return this->thread_list_;
}


bool operator== (const Process &n1, const Process &n2)
{
    return n1.process_id_ == n2.process_id_;
}


void Process::RemoveThread(ULONG thread_id)
{
    std::vector<Thread>::iterator it_thread = std::find(this->thread_list_.begin(), this->thread_list_.end(), thread_id);
    if (it_thread != this->thread_list_.end())
        this->thread_list_.erase(it_thread);
}
