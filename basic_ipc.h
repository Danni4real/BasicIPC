#ifndef BASICIPC_H
#define BASICIPC_H

#include <mutex>
#include <string>

#define PARALLEL_SYNC_MESSAGE_LIMIT 128 // when too many parallel sync messages, change this Macro to bigger value

struct Ack
{
    unsigned int  ack_id;
    std::string   ack_msg;
    bool          occupied;
    bool          msg_acked;
};

class BasicIPC
{
private:
    unsigned int ack_id;
    int          my_port;
    void*        msg_queue;

    std::mutex  ack_lock;
    Ack         ack_list[PARALLEL_SYNC_MESSAGE_LIMIT];

    unsigned int   new_ack();
    void           del_ack(int ack_id);
    void           set_ack(int ack_id, const std::string& ack_msg);
    bool           get_ack(int ack_id, std::string* ack_msg);

    bool  send(int dest_port, const std::string& msg);

    std::string (*sync_msg_handler)(int src_port, const std::string& msg);
    void       (*async_msg_handler)(int src_port, const std::string& msg);

    static void* msg_receiver(void* basic_ipc);
    static void* msg_processor(void* basic_ipc);

public:
    BasicIPC();

    // async_msg_handler and sync_msg_handler will not be called at same time
    void set_call_back(void(*async_msg_handler)(int, const std::string&), std::string(*sync_msg_handler)(int, const std::string&));

    // set_my_port() should be called after set_call_back(), if set_call_back() was called
    bool set_my_port(int my_port);

    // async_send is thread safe
    bool async_send(int dest_port, const std::string& msg);

    // sync_send is thread safe
    bool  sync_send(int dest_port, const std::string& msg, std::string* ack_msg, int time_out_millisec = 1000); // wait ack
};

#endif // BASICIPC_H
