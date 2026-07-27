#include "Helpers/TCPConnection.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <fcntl.h>

namespace Core {
    namespace Network {
        TCPServer::TCPServer(int port) {
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd == -1) {
                return;
            }
            
            addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);

            if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
                return;
            }

            success = true;
        }

        TCPServer::~TCPServer() {
            if (client_fd >= 0) close(client_fd);
            if (server_fd >= 0) close(server_fd);
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

        bool TCPServer::send_all(void *data, size_t size) {
            const char* ptr = reinterpret_cast<const char*>(data);
            size_t total_sent = 0;
            while (total_sent < size) {
                ssize_t sent = send(client_fd, ptr + total_sent, size - total_sent, 0);
                if (sent < 0) {
                    if (errno == EINTR)
                        continue;
                    return false;
                }
                if (sent == 0)
                    return false;
                total_sent += sent;
            }
            return true;
        }

        bool TCPServer::recv_all(void *buffer, size_t size) {
            size_t bytesRead = 0;
            while (bytesRead < size) {
                ssize_t len = read(client_fd, (char*)buffer + bytesRead, size - bytesRead);
                if (len == 0)
                    return false;
                if (len == -1) {
                    if (errno == EINTR)
                        continue;
                    return false;
                }
                bytesRead += len;
            }
            return bytesRead == size;
        }

        TCPClient::TCPClient() {
            if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                return;
            }

            serv_addr.sin_family = AF_INET;
            success = true;
        }
        
        bool TCPClient::Connect(const char* server_ip, u16 port) {
            serv_addr.sin_port = htons(port);

            if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
                return false;
            }

            if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                return false;
            }

            return true;
        }

        bool TCPClient::SendAll(const void* data, size_t size) {
            const char* ptr = reinterpret_cast<const char*>(data);
            size_t total_sent = 0;
            while (total_sent < size) {
                ssize_t sent = send(sock_fd, ptr + total_sent, size - total_sent, 0);
                
                if (sent == 0)
                    return false;

                if (sent < 0) {
                    if (errno == EINTR)
                        continue;

                    return false;
                }

                total_sent += sent;
            }
            return true;
        }

        bool TCPClient::RecvAll(void *buffer, size_t size) {
            size_t bytesRead = 0;
            while (bytesRead < size) {
                ssize_t len = recv(sock_fd, (char*)buffer + bytesRead, size - bytesRead, 0);

                if (len == 0)
                    return false;

                if (len == -1) {
                    if (errno == EINTR)
                        continue;

                    return false;
                }
                bytesRead += len;
            }
            return bytesRead == size;
        }

        TCPClient::~TCPClient() {
            if (sock_fd >= 0) close(sock_fd);
        }
    }
}