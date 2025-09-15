#include "Core/TCPConnection.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>

namespace Core {
    namespace Network {
        TCPServer::TCPServer(int port) {
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd == -1) {
                success = false;
                return;
            }
            
            addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);

            if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
                success = false;
                return;
            }

            success = true;
        }

        TCPServer::~TCPServer() {
            if (client_fd > 0) close(client_fd);
            if (server_fd > 0) close(server_fd);
            client_fd = -1;
            server_fd = -1;
        }

        std::string TCPServer::getHostName() {
            struct in_addr host_id;
            host_id.s_addr = gethostid();
            if (host_id.s_addr == INADDR_NONE)
                return "";
            return std::string(inet_ntoa(host_id));
        }

        bool TCPServer::waitConnection(WaitConnectionCallback callback) {
            aborted = false;
            if (listen(server_fd, 1) < 0)
                return false;
            fd_set set;
            timeval timeout;

            while (true) {
                if (callback != nullptr && !callback()) {
                    aborted = true;
                    return false;
                }

                FD_ZERO(&set);
                FD_SET(server_fd, &set);
                int waitMs = 100;
                timeout.tv_sec = waitMs / 1000;
                timeout.tv_usec = (waitMs % 1000) * 1000;

                int rv = select(server_fd + 1, &set, nullptr, nullptr, &timeout);
                if (rv == -1)
                    return false;
                else if (rv == 0)
                    continue;
                else {
                    client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd < 0)
                        return false;
                    break;
                }
            }
            return true;
        }

        bool TCPServer::send(void *data, size_t size) {
            return size == write(client_fd, data, size);
        }

        bool TCPServer::recv(void *buffer, size_t size) {
            size_t bytesRead = 0;
            while (bytesRead < size) {
                size_t len = read(client_fd, (char*)buffer + bytesRead, size - bytesRead);
                if (len <= 0)
                    return false;
                bytesRead += len;
            }
            return bytesRead == size;
        }
    }
}