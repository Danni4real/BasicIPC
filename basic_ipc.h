#ifndef BASICIPC_H
#define BASICIPC_H

#include <string>

class BasicIPC
{
private:
    class BasicIPC_Private;
    BasicIPC_Private* p;

public:
    BasicIPC(unsigned int my_port);

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
