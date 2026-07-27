#pragma once

#include "CoreGlobals.hpp"

#include <stddef.h>
#include <netinet/in.h>

namespace Core {
    namespace Network {
        class TCPServer {
            using WaitConnectionCallback = bool(*)(void);

            int server_fd = -1;
            int client_fd = -1;
            sockaddr_in addr;
            bool success = false;
            
            public:
                bool aborted = false;
            
            public:
                TCPServer(int port);
                ~TCPServer();

                std::string getHostName();

                bool waitConnection(WaitConnectionCallback callback = nullptr);

                bool send_all(void *data, size_t size);

                bool recv_all(void *buffer, size_t size);
        };

        class TCPClient {
            int sock_fd = -1;
            bool success = false;
            sockaddr_in serv_addr{};

            public:
                TCPClient();
                ~TCPClient();

                bool Connect(const char* server_ip, u16 port);

                bool SendAll(const void* data, size_t size);

                bool RecvAll(void *buffer, size_t size);

                bool isReady() {
                    return success;
                }
        };
    }
}