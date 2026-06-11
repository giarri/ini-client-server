#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iniresource.h>
#include <csignal>

static constexpr int PORT     = 12345;
static constexpr int BACKLOG  = 8;
static constexpr int BUF_SIZE = 4096; //max value size
static volatile sig_atomic_t g_running = 1;

void handle_sigint(int);
static unsigned short serve_client(int fd);
static bool read_line(int fd, std::string& out);
static std::string handle_command(const std::string& line);

int main() {
    struct sigaction sa{};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
#ifdef SA_INTERRUPT
    sa.sa_flags = SA_INTERRUPT;
#endif
    sigaction(SIGINT, &sa, nullptr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    const int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen"); return 1;
    }

    std::cout << "Listening on port " << PORT << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) break;   // interrupted by SIGINT
            perror("accept");
            continue;
        }
        serve_client(client_fd);
        std::cout << "Client on port "<< client_addr.sin_port << " connected\n";
    }
    close(server_fd);
    return 0;
}

void handle_sigint(int) {
    g_running = 0;
}


// Handle a single null-terminated command string; writes response into `out`.
static std::string handle_command(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "LOAD") {
        std::string path;
        if (!(iss >> path)) goto error;
        int rc = load_resource(path);
        return std::to_string(rc) + "\n";
    }

    if (cmd == "GET") {
        std::string key;
        if (!(iss >> key)) goto error;
        std::string val;
        int rc = get_value(key, val);
        return std::to_string(rc) + " " + val + "\n";
    }

    if (cmd == "SET") {
        std::string key;
        if (!(iss >> key)) goto error;

        std::string value;
        std::getline(iss >> std::ws, value);

        if (value.empty())  goto error;
        int rc = set_value(key, value);
        return std::to_string(rc) + "\n";
    }
    error:
        return "127\n";
}

static bool read_line(int fd, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = read(fd, &ch, 1);
        if (n <= 0) return false;
        if (ch == '\n') return true;
        out += ch;
    }
}

/**
 *
 * @param fd client file descriptor  for communication
 * @ret 1 - operation succeeded\n
 *      <0 error code
 */
static unsigned short serve_client(int fd) {
    std::string line;
    while (read_line(fd, line)) {
        std::string response = handle_command(line);
        write(fd, response.c_str(), response.size());
    }
    close(fd);
    return 1;
}
