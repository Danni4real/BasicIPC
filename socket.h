#ifndef SOCKET_H
#define SOCKET_H

#include <sys/un.h>
#include <string>

class Socket
{
private:

    int send_sock;
    int recv_sock;

    char recv_buf[1024];

    struct sockaddr_un   my_sock_addr;
    struct sockaddr_un dest_sock_addr;

public:

    int send(std::string buf);
    int send(const char* buf);
    int recv();

    char* get_recv_buf();

    bool open_send_sock(std::string dest_sock_file_path);
    bool open_recv_sock(std::string   my_sock_file_path);

    bool open_send_sock(const char* dest_sock_file_path);
    bool open_recv_sock(const char*   my_sock_file_path);

    bool close_send_sock();
    bool close_recv_sock();
};

#endif // SOCKET_H
