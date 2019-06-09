#ifndef OS_NET_DESCRIPTOR_PASSING_SERVER_H
#define OS_NET_DESCRIPTOR_PASSING_SERVER_H

#include <iostream>
#include <cstring>
#include <vector>
#include <errno.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


#include "fd.h"

const std::string HELP = R"SEQ(Usage:
    -help           - usage this program
    <address>       - socket address, default address = "default address"
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

struct server {
    explicit server(const std::string &addr);

    void run();

private:
    sockaddr_un socket_addr;
    fd_wrapper socket_fd;
};

int main(int argc, char* argv[]);

#endif // OS_NET_DESCRIPTOR_PASSING_SERVER_H
