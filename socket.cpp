#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>

#include "socket.h"

bool Socket::open_send_sock(const std::string& dest_sock_file_path)
{
    return open_send_sock(dest_sock_file_path.c_str());
}
bool Socket::open_recv_sock(const std::string& my_sock_file_path)
{
    return open_recv_sock(my_sock_file_path.c_str());
}

bool Socket::open_send_sock(const char* dest_sock_file_path)
{
    send_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(send_sock == -1)
    {
        perror("Socket: open_send_sock(): socket()");
        return false;
    }

    memset(&dest_sock_addr, 0, sizeof(dest_sock_addr));

    dest_sock_addr.sun_family = AF_UNIX;
    strcpy(dest_sock_addr.sun_path, dest_sock_file_path);

    return true;
}

bool Socket::open_recv_sock(const char* my_sock_file_path)
{
    recv_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(recv_sock == -1)
    {
        perror("Socket: open_recv_sock(): socket()");
        return false;
    }

    memset(&my_sock_addr, 0, sizeof(my_sock_addr));

    my_sock_addr.sun_family = AF_UNIX;
    strcpy(my_sock_addr.sun_path, my_sock_file_path);
    unlink(my_sock_file_path);

    int ret = bind(recv_sock, (struct sockaddr*) &my_sock_addr, sizeof(my_sock_addr));
    if(ret == -1)
    {
        perror("Socket: open_recv_sock(): bind()");
        close(recv_sock);
        return false;
    }

    return true;
}

int Socket::send(const std::string& buf)
{
    return send(buf.c_str());
}

int Socket::send(const char* buf)
{
    int sent_len = sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr *) &dest_sock_addr, sizeof(dest_sock_addr));

    if(sent_len == -1)
        perror("Socket: send(): sendto()");
    else if(sent_len < (int)strlen(buf))
        printf("Socket: send(): sent %d bytes instead of %d bytes!\n", sent_len, (int)strlen(buf));

    return sent_len;
}

int Socket::recv()
{
    struct sockaddr_un src_sock_addr;
    int sock_addr_len = sizeof(my_sock_addr);

    memset(recv_buf, 0, sizeof(recv_buf));

    int recvd_len = recvfrom(recv_sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &src_sock_addr, (socklen_t *) &sock_addr_len);
    if(recvd_len == -1)
        perror("Socket: recv(): recvfrom()");

    return recvd_len;
}

char* Socket::get_recv_buf()
{
    return recv_buf;
}

bool Socket::close_send_sock()
{
    if(close(send_sock) == -1)
    {
        perror("Socket: close_send_sock(): close()");
        return false;
    }

    return true;
}

bool Socket::close_recv_sock()
{
    if(close(recv_sock) == -1)
    {
        perror("Socket: close_recv_sock(): close()");
        return false;
    }

    return true;
}
