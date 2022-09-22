#include <list>

#include "message_buf.h"

bool MessageBuf::is_empty()
{
    return buf.empty();
}

void MessageBuf::clear()
{
    buf_lock.lock();

    buf.clear();

    buf_lock.unlock();
}

std::string MessageBuf::pop()
{
    buf_lock.lock();

    std::string message = buf.front();

    buf.pop_front();

    buf_lock.unlock();

    return message;
}

void MessageBuf::push(std::string message)
{
    buf_lock.lock();

    buf.push_back(message);

    buf_lock.unlock();
}
