#ifndef BASICIPC_H
#define BASICIPC_H

#include <mutex>
#include <string>

// if too many parallel sync messages, change PARALLEL_SYNC_MESSAGE_LIMIT to bigger value, then recompile BasicIPC!!!
#define PARALLEL_SYNC_MESSAGE_LIMIT 128

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
    BasicIPC(int my_port);

    // async_msg_handler() and sync_msg_handler() will not be called at same time
    bool set_call_back(      void(*async_msg_handler)(int src_port, const std::string& msg),
                       std::string(*sync_msg_handler)(int src_port, const std::string& msg));

    // listen_my_port() should be called after set_call_back()
    bool listen_my_port();

    // async_send and sync_send are thread safe
    bool async_send(int dest_port, const std::string& msg);
    bool  sync_send(int dest_port, const std::string& msg, std::string* ack_msg, int time_out_millisec = 1000);
};

#endif // BASICIPC_H
