#ifndef OS_NET_DESCRIPTOR_PASSING_CLIENT_H
#define OS_NET_DESCRIPTOR_PASSING_CLIENT_H

#include <iostream>
#include <errno.h>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "fd.h"

const std::string HELP = R"SEQ(Usage:
    You could send small message to server 
    -help           - usage this program
    <address>       - socket address, default address = "default address"
    -exit           - exit client
)SEQ";

const ssize_t MAX_LEN_MESSAGE = 1 << 10;

void error(const std::string &message, bool with_errno = true, bool help = false, bool need_exit = false) {
    std::cerr << message;
    
    if (with_errno) {
        std::cerr << ". " << strerror(errno);
    }
    std::cerr << std::endl;

    if (help) {
        std::cerr << HELP << std::endl;
    }

    if (need_exit) {
        exit(EXIT_FAILURE);
    }
}

struct client {
    explicit client(const std::string &addr);

    void run();

  private:
    sockaddr_un socket_addr;
    fd_wrapper socket_client_fd;

    fd_wrapper fd_for_read;
    fd_wrapper fd_for_write;
};

int main(int argc, char* argv[]);

#endif // OS_NET_DESCRIPTOR_PASSING_CLIENT_H
