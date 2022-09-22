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

BasicIPC::BasicIPC()
{
    ack_id  =  0;
    my_port = -1;

    sync_msg_handler = 0;
   async_msg_handler = 0;

   msg_queue = new MessageBuf();

   for(int i=0; i< PARALLEL_SYNC_MESSAGE_LIMIT; i++)
       ack_list[i].occupied  = false;
}

unsigned int BasicIPC::new_ack()
{
    unsigned int new_ack_id;

    ack_lock.lock();
    new_ack_id = this->ack_id++;
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

void BasicIPC::del_ack(int ack_id)
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

void BasicIPC::set_ack(int ack_id, const string& ack_msg)
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
bool BasicIPC::get_ack(int ack_id, std::string* ack_msg)
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


bool BasicIPC::set_my_port(int my_port)
{
    int       ret;
    pthread_t tid;

    this->my_port = my_port;

    ret = pthread_create(&tid, NULL, msg_receiver, this);
    if(ret != 0)
    {
        cout << "BasicIPC: msg_receiver thread create failed!" << endl;
        return false;
    }

    ret = pthread_create(&tid, NULL, msg_processor, this);
    if(ret != 0)
    {
        cout << "BasicIPC: msg_processor thread create failed!" << endl;
        return false;
    }

    return true;
}

void* BasicIPC::msg_processor(void* basic_ipc)
{
    BasicIPC*    self = (BasicIPC*)basic_ipc;
    MessageBuf* msg_q = (MessageBuf*)self->msg_queue;

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
            string sync_id = get_field(msg,2);

            if(self->sync_msg_handler != 0)
            {
                string ack = self->sync_msg_handler(src_port, remove_head(remove_head(remove_head(msg))));

                self->send(src_port, add_head(ACK_MESSAGE_HEAD, add_head(sync_id, ack)));
            }
        }
        if(msg_head == ASYNC_MESSAGE_HEAD)
        {
            if(self->async_msg_handler != 0)
                self->async_msg_handler(src_port, remove_head(remove_head(msg)));
        }
    }
}

void* BasicIPC::msg_receiver(void* basic_ipc)
{
    BasicIPC*    self = (BasicIPC*)basic_ipc;
    MessageBuf* msg_q = (MessageBuf*)self->msg_queue;

    bool   ret;
    Socket sock;
    ret =  sock.open_recv_sock(string(SOCKET_FILE_PREFIX) + to_string(self->my_port));
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

        cout << "Recv: " << msg << endl;

        if(msg_head == ACK_MESSAGE_HEAD)
        {
            int    ack_id  = to_int(get_field(msg,1));
            string ack_msg = remove_head(remove_head(msg));

            self->set_ack(ack_id, ack_msg);
        }
        else
        {
            msg_q->push(msg);
        }
    }
}

bool BasicIPC::send(int dest_port, const string& msg)
{
    bool   ret;
    Socket sock;

    ret = sock.open_send_sock(string(SOCKET_FILE_PREFIX) + to_string(dest_port));
    if(ret)
    {
        if(sock.send(msg) < msg.length())
            ret = false;

        sock.close_send_sock();
    }

    return ret;
}

bool BasicIPC::sync_send(int dest_port, const string& msg, string* ack_msg, int time_out_millisec)
{
    bool          ret;
    unsigned int  ack_id;

    ack_id = new_ack();

    string message = add_head(SYNC_MESSAGE_HEAD, add_head(my_port, add_head(ack_id, msg)));

    cout << "Send: " << message << endl;

    ret = send(dest_port, message);
    if(ret)
    {
        for(int i=0; i<time_out_millisec; i++)
        {
            if(get_ack(ack_id, ack_msg) == true)
                goto end;

            usleep(1000);
        }

        cout << "BasicIPC: sync_send(): timeout!" << endl;
        ret = false;
    }

end:
    del_ack(ack_id);
    return ret;
}

bool BasicIPC::async_send(int dest_port, const string& msg)
{
    bool ret;

    string message = add_head(ASYNC_MESSAGE_HEAD, add_head(my_port, msg));
    ret = send(dest_port, message);

    return ret;
}

void BasicIPC::set_call_back(void(*async_msg_handler)(int, const string&), string(*sync_msg_handler)(int, const string&))
{
     this->sync_msg_handler =  sync_msg_handler;
    this->async_msg_handler = async_msg_handler;
}
