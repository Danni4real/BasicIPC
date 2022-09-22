#ifndef MESSAGE_BUF_H
#define MESSAGE_BUF_H

#include <list>
#include <mutex>
#include <string>

class MessageBuf
{
private:

    std::list<std::string> buf;
    std::mutex             buf_lock;

public:

    bool is_empty();
    void clear();

    std::string pop();
    void        push(std::string message);
};

#endif // MESSAGE_BUF_H
