#include "server.h"

server::server(const std::string &addr) : socket_fd(socket(AF_UNIX, SOCK_SEQPACKET, 0)) {
    if (socket_fd.get_fd() == -1) {
        throw std::runtime_error("Can't create socket");
    }

    memset(&socket_addr, 0, sizeof(sockaddr_un));

    std::string real_addr = '\0' + addr;
    strncpy(socket_addr.sun_path, real_addr.c_str(), sizeof(socket_addr.sun_path) - 1);
    socket_addr.sun_family = AF_UNIX;

    if (bind(socket_fd.get_fd(), (sockaddr *) (&socket_addr), sizeof(struct sockaddr_un)) == -1) {
        throw std::runtime_error("Can't bind socket");
    }

    if (listen(socket_fd.get_fd(), 5) == -1) {
        throw std::runtime_error("Can't listen socket");
    }
}

void close_fd(int fd) {
    if (fd != -1) {
        if (close(fd) == -1) {
            error("Can't close fd");
        }
    }
}

bool success_send_fd_to_client(int client_fd, int fd_for_send1, int fd_for_send2) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char iobuf[CMSG_SPACE(sizeof(int[2]))];
    char c;

    struct iovec io[1];
    io[0].iov_base = &c;
    io[0].iov_len = sizeof(c);

    memset(iobuf, 0x0c, sizeof(iobuf));

    cmsg = (struct cmsghdr *) (iobuf);

    cmsg->cmsg_len = CMSG_LEN(sizeof(int[2]));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = io;
    msg.msg_iovlen = sizeof(io) / sizeof(io[0]);
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int[2]));
    msg.msg_flags = 0;
    int *data = (int *) (CMSG_DATA(cmsg));

    data[0] = fd_for_send1;
    data[1] = fd_for_send2;
    if (sendmsg(client_fd, &msg, 0) == -1) {
        error("Sendmsg failed");

        close_fd(fd_for_send1);
        close_fd(fd_for_send2);
        return false;
    }

    close_fd(fd_for_send1);
    close_fd(fd_for_send2);
    return true;
}

void server::run() {
    while (true) {
        std::cout << "Сonnection..." << std::endl;

        fd_wrapper client_fd(accept(socket_fd.get_fd(), nullptr, nullptr));
        if (client_fd.get_fd() == -1) {
            error("Can't accept client address");
            continue;
        }

        std::cout << "Client connected" << std::endl;

        int pipe_fd_read_server[2];

        if (pipe(pipe_fd_read_server) == -1) {
            error("Can't create pipe for read");
            continue;
        }

        int pipe_fd_write_server[2];

        if (pipe(pipe_fd_write_server) == -1) {
            error("Can't create pipe for write");
            continue;
        }

        if (!success_send_fd_to_client(client_fd.get_fd(), pipe_fd_read_server[1], pipe_fd_write_server[0])) {
            error("Can't send fds");
            close_fd(pipe_fd_read_server[0]);
            close_fd(pipe_fd_write_server[1]);
            continue;
        }

        while (true) {
            std::string message(MAX_LEN_MESSAGE, ' ');

            ssize_t read_message_length = read(pipe_fd_read_server[0], (void *) message.data(), MAX_LEN_MESSAGE);

            if (read_message_length > 0) {
                message.resize(read_message_length);
                ssize_t write_message_length = write(pipe_fd_write_server[1],
                                                     message.data(), read_message_length);

                if (write_message_length <= 0) {
                    if (write_message_length == -1) {
                        error("Can't send message");
                    }

                    std::cout << "Client disconnected" << std::endl;
                    break;
                }
            } else {
                if (read_message_length == -1) {
                    error("Can't read message");
                }

                std::cout << "Client disconnected" << std::endl;
                break;
            }
        }

        close_fd(pipe_fd_read_server[0]);
        close_fd(pipe_fd_write_server[1]);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 2 && argv[1] != nullptr && (strcmp(argv[1], "-help") == 0)) {
        std::cerr << HELP << std::endl;
        return 0;
    }
    if (argc > 2) {
        error("Wrong usage", false, true, true);
    }

    try {
        std::string address = "default address";

        if (argc == 2) {
            address = argv[1];
        }

        server server(address);
        server.run();
    } catch (const std::invalid_argument &e) {
        error(e.what(), true, true, true);
    } catch (const std::runtime_error &e) {
        error(e.what(), true, false, true);
    }
}
