#include "Thread.h"


Thread::Thread(ItemType state, ULONG thread_id, ULONG process_owner_id, SYSTEMTIME st) :
    state_(state),
    thread_id_(thread_id),
    process_owner_id_(process_owner_id),
    st_(st)
{}

Thread::Thread(ULONG thread_id) :
    thread_id_(thread_id)
{}


Thread::~Thread() = default;


const ItemType Thread::State()
{
    return this->state_;
}


const ULONG Thread::ThreadID()
{
    return this->thread_id_;
}


const ULONG Thread::ProcessOwner()
{
    return this->process_owner_id_;
}


const SYSTEMTIME Thread::ST()
{
    return this->st_;
}


bool operator== (const Thread &n1, const Thread &n2)
{
    return n1.thread_id_ == n2.thread_id_;
}
