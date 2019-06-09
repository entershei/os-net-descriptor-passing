#include "client.h"


std::pair<int, int> read_fd_from_server(int client_fd) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char iobuf[CMSG_SPACE(sizeof(int[2]))];
    char c;

    struct iovec io[1];
    io[0].iov_base = &c;
    io[0].iov_len = sizeof(c);

    memset(iobuf, 0x0a, sizeof(iobuf));

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

    if (recvmsg(client_fd, &msg, 0) == -1) {
        error("Recvmsg failed");
        return {-1, -1};
    }

    int *fds = (int *) (CMSG_DATA(cmsg));

    return {fds[0], fds[1]};
}

client::client(const std::string &addr) : socket_client_fd(socket(AF_UNIX, SOCK_SEQPACKET, 0)), fd_for_write(-1),
                                          fd_for_read(-1) {
    if (socket_client_fd.get_fd() == -1) {
        throw std::runtime_error("Can't create socket");
    }

    memset(&socket_addr, 0, sizeof(struct sockaddr_un));

    socket_addr.sun_family = AF_UNIX;
    std::string real_addr = '\0' + addr;
    strncpy(socket_addr.sun_path, real_addr.c_str(), sizeof(socket_addr.sun_path) - 1);

    if (connect(socket_client_fd.get_fd(), (sockaddr *) (&socket_addr), sizeof(struct sockaddr_un)) == -1) {
        throw std::runtime_error("Can't connect to server");
    }
}

void client::run() {
    auto fds = read_fd_from_server(socket_client_fd.get_fd());

    fd_for_write.assign_fd(fds.first);
    if (fd_for_write.get_fd() == -1) {
        throw std::runtime_error("Can't read fd of write pipe");
    }

    fd_for_read.assign_fd(fds.second);
    if (fd_for_read.get_fd() == -1) {
        throw std::runtime_error("Can't read fd of read pipe");
    }

    std::cout << "Echo-client: type message" << std::endl;

    while (!std::cin.eof()) {
        std::cout << "Request:  ";
        std::cout.flush();

        std::string message;
        std::getline(std::cin, message);

        if (message == "-exit") {
            break;
        }

        if (message == "\n" || message.empty()) {
            continue;
        }

        if (message.length() > MAX_LEN_MESSAGE) {
            error("Too large message", false);
            continue;
        }

        size_t message_length = message.length();
        ssize_t write_message_length = write(fd_for_write.get_fd(), message.data(), message_length);
        if (write_message_length > 0) {
            std::string response(message_length, ' ');
            ssize_t read_message_length = read(fd_for_read.get_fd(), (void *) response.data(), message_length);

            if (read_message_length <= 0) {
                throw std::runtime_error("Can't send message");
            }

            std::cout << "Response: " << response << std::endl;
        } else {
            throw std::runtime_error("Can't send message");
        }
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc == 2 && argv[1] != nullptr && (strcmp(argv[1], "-help") == 0)) {
        std::cerr << HELP << std::endl;
        return 0;
    } else if (argc == 2 && argv[1] != nullptr && (strcmp(argv[1], "-exit") == 0)) {
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

        client client(address);

        client.run();
    } catch (std::runtime_error &e) {
        error(e.what(), true, false, true);
    }

    return 0;
}
