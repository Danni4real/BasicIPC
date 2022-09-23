#include <string>
#include <unistd.h>
#include <iostream>

#include "basic_ipc.h"

using namespace std;

void handle_async_msg(int src_port, const string& msg)
{
    cout << "handle_async_msg: " << msg << endl;
}

string handle_sync_msg(int src_port, const string& msg)
{
    cout << "handle_sync_msg: " << msg << endl;


    return msg + " ack";
}

enum PortList
{
    Port_1,
    Port_2
};

BasicIPC usr_1(Port_1);
BasicIPC usr_2(Port_2);

#define U_1
//#define U_2

#ifdef U_1
int main()
{
    usr_1.set_call_back(handle_async_msg, handle_sync_msg);
    usr_1.listen_my_port();

    usr_1.async_send(Port_2, "Async Message");

    string ack;
    usr_1.sync_send(Port_2, "Sync Message", &ack);
    cout << ack << endl;

    return 0;
}
#endif
#ifdef U_2
int main()
{
    usr_2.set_call_back(handle_async_msg, handle_sync_msg);
    usr_2.listen_my_port();

    while(true)
        sleep(1);

    return 0;
}
#endif
