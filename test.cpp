#include <string>
#include <unistd.h>
#include <iostream>

#include "basic_ipc.h"

using namespace std;

enum PortList
{
    Port_1,
    Port_2,
    Port_3,
    Port_4,

    Port_Num
};

BasicIPC usr_1;
BasicIPC usr_2;
BasicIPC usr_3;

void handle_async_msg(int src_port, const string& msg)
{
    cout << "handle_async_msg: " << msg << endl;
}

string handle_sync_msg_1(int src_port, const string& msg)
{
    cout << "handle_sync_msg: " << msg << endl;

    string ack;
    usr_1.sync_send(Port_3, msg+"1", &ack);
    cout << ack << endl;

    return msg + " 1";
}

string handle_sync_msg_2(int src_port, const string& msg)
{
    cout << "handle_sync_msg: " << msg << endl;

    string ack;
    usr_2.sync_send(Port_1, msg+"2", &ack);
    cout << ack << endl;

    return msg + " 2";
}

string handle_sync_msg_3(int src_port, const string& msg)
{
    cout << "handle_sync_msg: " << msg << endl;

    return msg + " 3";
}

#define U_1
//#define U_2
//#define U_3


#ifdef U_1
int main()
{
    usr_1.set_call_back(handle_async_msg, handle_sync_msg_1);
    usr_1.set_my_port(Port_1);

    string ack;
    usr_1.sync_send(Port_2, "fuck you", &ack);
    cout << ack << endl;

    return 0;
}
#endif
#ifdef U_2
int main()
{
    usr_2.set_call_back(handle_async_msg, handle_sync_msg_2);
    usr_2.set_my_port(Port_2);

    while(true)
        sleep(1);

    return 0;
}
#endif
#ifdef U_3
int main()
{
    usr_3.set_call_back(handle_async_msg, handle_sync_msg_3);
    usr_3.set_my_port(Port_3);

    while(true)
        sleep(1);

    return 0;
}
#endif
