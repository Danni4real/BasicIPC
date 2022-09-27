#include <unistd.h>

#include <mutex>
#include <string>
#include <iostream>

#include "socket.h"
#include "basic_ipc.h"
#include "message_buf.h"

#define   ACK_MESSAGE_HEAD   "#ACK"
#define  SYNC_MESSAGE_HEAD  "#SYNC"
#define ASYNC_MESSAGE_HEAD "#ASYNC"

#define SOCKET_FILE_PREFIX  "/tmp/BasicIPC_USR_"

// if too many parallel sync messages, change PARALLEL_SYNC_MESSAGE_LIMIT to bigger value, then recompile BasicIPC!!!
#define PARALLEL_SYNC_MESSAGE_LIMIT 128

#define Debug

using namespace std;

int to_int(const string& arg)
{
    return atoi(arg.c_str());
}

string add_head(int head, const string& msg)
{
    string _head = to_string(head);

    return _head + ',' + msg;
}

string add_head(const char* head, const string& msg)
{
    string _head = head;

    return _head + ',' + msg;
}

string add_head(const string& head, const string& msg)
{
    return head + ',' + msg;
}

string remove_head(const string& msg)
{
    return msg.substr(msg.find(',')+1);
}

string get_field(string str, int index)
{
    int pos;
    string field;

    while(index >= 0)
    {
        pos   = str.find(',');
        field = str.substr(0, pos);
        str   = str.substr(pos+1);

        index--;
    }

    return field;
}


struct Ack
{
    unsigned int  ack_id;
    std::string   ack_msg;
    bool          occupied;
    bool          msg_acked;
};

class BasicIPC::BasicIPC_Private
{
public:
    unsigned int ack_id;
    unsigned int my_port;
    void*        msg_queue;

    mutex       ack_lock;
    Ack         ack_list[PARALLEL_SYNC_MESSAGE_LIMIT];

    unsigned int   new_ack();
    void           del_ack(int ack_id);
    void           set_ack(int ack_id, const string& ack_msg);
    bool           get_ack(int ack_id,       string* ack_msg);

    bool  send(int dest_port, const string& msg);

    string      (*sync_msg_handler)(int src_port, const string& msg);
    void       (*async_msg_handler)(int src_port, const string& msg);

    static void* msg_receiver(void*  self);
    static void* msg_processor(void* self);
};

unsigned int BasicIPC::BasicIPC_Private::new_ack()
{
    unsigned int new_ack_id;

    ack_lock.lock();
    new_ack_id = ack_id++;
    ack_lock.unlock();

    while(true)
    {
        ack_lock.lock();
        for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
        {
            if(ack_list[i].occupied == false)
            {
                ack_list[i].occupied  = true;
                ack_list[i].ack_id    = new_ack_id;
                ack_list[i].msg_acked = false;

                ack_lock.unlock();
                goto end;
            }
        }
        ack_lock.unlock();

        cout << "BasicIPC: new_ack(): too many parallel sync messages, change PARALLEL_SYNC_MESSAGE_LIMIT to bigger value!!!" << endl;
        usleep(100000);
    }

end:
    return new_ack_id;
}

void BasicIPC::BasicIPC_Private::del_ack(int ack_id)
{
    ack_lock.lock();

    for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
    {
        if(ack_list[i].occupied && ack_list[i].ack_id == ack_id)
        {
            ack_list[i].occupied  = false;
            break;
        }
    }

    ack_lock.unlock();
}

void BasicIPC::BasicIPC_Private::set_ack(int ack_id, const string& ack_msg)
{
    ack_lock.lock();

    for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
    {
        if(ack_list[i].occupied && ack_list[i].ack_id == ack_id)
        {
            ack_list[i].ack_msg   = ack_msg;
            ack_list[i].msg_acked = true;
            break;
        }
    }

    ack_lock.unlock();
}
bool BasicIPC::BasicIPC_Private::get_ack(int ack_id, std::string* ack_msg)
{
    bool ret = false;

    ack_lock.lock();

    for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
    {
        if(ack_list[i].occupied && ack_list[i].ack_id == ack_id)
        {
            if(ack_list[i].msg_acked)
            {
                *ack_msg = ack_list[i].ack_msg;
                ret = true;
            }
            break;
        }
    }

    ack_lock.unlock();

    return ret;
}

void* BasicIPC::BasicIPC_Private::msg_processor(void* self)
{
    BasicIPC_Private* p = (BasicIPC_Private*)self;
    MessageBuf*   msg_q = (MessageBuf*)p->msg_queue;

    while(true)
    {
        if(msg_q->is_empty())
        {
            usleep(1000);
            continue;
        }

        string msg      = msg_q->pop();
        string msg_head =        get_field(msg,0);
        int    src_port = to_int(get_field(msg,1));

        if(msg_head == SYNC_MESSAGE_HEAD)
        {
            if(p->sync_msg_handler != 0)
            {
                string sync_id = get_field(msg,2);

                string ack = p->sync_msg_handler(src_port, remove_head(remove_head(remove_head(msg))));

                p->send(src_port, add_head(ACK_MESSAGE_HEAD, add_head(sync_id, ack)));
            }
        }
        if(msg_head == ASYNC_MESSAGE_HEAD)
        {
            if(p->async_msg_handler != 0)
                p->async_msg_handler(src_port, remove_head(remove_head(msg)));
        }
    }
}

void* BasicIPC::BasicIPC_Private::msg_receiver(void* self)
{
    BasicIPC_Private* p = (BasicIPC_Private*)self;
    MessageBuf*   msg_q = (MessageBuf*)p->msg_queue;

    bool   ret;
    Socket sock;
    ret =  sock.open_recv_sock(string(SOCKET_FILE_PREFIX) + to_string(p->my_port));
    if(!ret)
    {
        cout << "BasicIPC: msg_receiver(): open_recv_sock() failed!" << endl;
        return 0;
    }

    while(true)
    {
        if(sock.recv() <= 0)
            continue;

        string msg      = sock.get_recv_buf();
        string msg_head = get_field(msg,0);

        if(msg_head == ACK_MESSAGE_HEAD)
        {
#ifdef Debug
            cout << "Recv ack: " << msg << endl;
#endif
            int    ack_id  = to_int(get_field(msg,1));
            string ack_msg = remove_head(remove_head(msg));

            p->set_ack(ack_id, ack_msg);
        }
        else
        {
#ifdef Debug
            cout << "Recv from Port " << get_field(msg,1) << ": " << msg << endl;
#endif
            msg_q->push(msg);
        }
    }
}

bool BasicIPC::BasicIPC_Private::send(int dest_port, const string& msg)
{
    bool   ret;
    Socket sock;
#ifdef Debug
    cout << "Send to Port "<< dest_port << ": " << msg << endl;
#endif
    ret = sock.open_send_sock(string(SOCKET_FILE_PREFIX) + to_string(dest_port));
    if(ret)
    {
        if(sock.send(msg) < msg.length())
            ret = false;

        sock.close_send_sock();
    }

    return ret;
}

BasicIPC::BasicIPC(unsigned int my_port)
{
    p = new BasicIPC_Private();

    p->my_port = my_port;

    p->ack_id  =  0;

    p->sync_msg_handler = 0;
   p->async_msg_handler = 0;

   p->msg_queue = new MessageBuf();

   for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
       p->ack_list[i].occupied  = false;
}

bool BasicIPC::listen_my_port()
{
    int       ret;
    pthread_t tid;

    if(p->sync_msg_handler == 0 || p->async_msg_handler == 0)
    {
        cout << "BasicIPC: call set_call_back() before call listen_my_port()!" << endl;
        return false;
    }

    ret = pthread_create(&tid, NULL, p->msg_receiver, p);
    if(ret != 0)
    {
        cout << "BasicIPC: msg_receiver thread create failed!" << endl;
        return false;
    }

    ret = pthread_create(&tid, NULL, p->msg_processor, p);
    if(ret != 0)
    {
        cout << "BasicIPC: msg_processor thread create failed!" << endl;
        return false;
    }

    return true;
}

bool BasicIPC::sync_send(int dest_port, const string& msg, string* ack_msg, int time_out_millisec)
{
    bool          ret;
    unsigned int  ack_id;

    ack_id = p->new_ack();

    string message = add_head(SYNC_MESSAGE_HEAD, add_head(p->my_port, add_head(ack_id, msg)));

    ret = p->send(dest_port, message);
    if(ret)
    {
        for(int i=0; i<time_out_millisec; i++)
        {
            if(p->get_ack(ack_id, ack_msg) == true)
                goto end;

            usleep(1000);
        }

        cout << "BasicIPC: sync_send(): timeout!" << endl;
        ret = false;
    }

end:
    p->del_ack(ack_id);
    return ret;
}

bool BasicIPC::async_send(int dest_port, const string& msg)
{
    bool ret;

    string message = add_head(ASYNC_MESSAGE_HEAD, add_head(p->my_port, msg));

    ret = p->send(dest_port, message);

    return ret;
}

bool BasicIPC::set_call_back( void(*async_msg_handler)(int, const string&),
                             string(*sync_msg_handler)(int, const string&))
{
    if(async_msg_handler == 0 || sync_msg_handler == 0)
    {
        cout << "BasicIPC: set_call_back(): can't set 0 as call back function pointer!" << endl;
        return false;
    }

     p->sync_msg_handler =  sync_msg_handler;
    p->async_msg_handler = async_msg_handler;

    return true;
}
