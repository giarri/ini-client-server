#include <iostream>
#include <ostream>
#include <set>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define HOST "127.0.0.1"
#define PORT 12345
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define BUF_SIZE 2048
typedef int SOCKET;

int connect();
void send_command(const std::string & cmd);

int main (int argc, char* argv[]) {
    if (argc < 3) {
    print_usage_and_exit:
        std::cout << "Usage: " << argv[0] <<
            __FILE_NAME__ << " --load <ini file>\n"
            __FILE_NAME__ << " --get <key>\n"
            __FILE_NAME__ << " --set <key> <value>\n"
            << std::endl;
        return 1;
    }
    std::string cmd;
    if (std::string(argv[1]) == "--load") {
        cmd = "LOAD " + std::string(argv[2]) + "\n";
    } else if (std::string(argv[1]) == "--get") {
        cmd = "GET " + std::string(argv[2]) + "\n";
    } else if (argc >= 4 && std::string(argv[1]) == "--set") {
        std::string value;
        for (int i = 3; i < argc; ++i)
            value += " " + std::string(argv[i]);
        cmd = "SET " + std::string(argv[2]) + " " + value + "\n";
    } else {
        goto print_usage_and_exit;
    }
    send_command(cmd);
    return 0;
}

void send_command(const std::string& cmd) {
    int socket = connect();
    if (socket == -1) {
        return;
    }
    if (send(socket, cmd.c_str(), cmd.length(), 0)<0) {
        close(socket);
        return;
    }
    char *buf = new char[BUF_SIZE];
    int bytes = recv(socket, buf, BUF_SIZE, 0);
    if (bytes<0){goto fail_safe;}
    buf[bytes] = '\0';
    std::cout << buf << std::endl;
    fail_safe:
        close(socket);
}

int connect() {
    const char* server_ip = HOST;
    int port = PORT;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        return -1;
    }
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported.\n";
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed.\n";
        close(sock);
        return -1;
    }
    return sock;
}