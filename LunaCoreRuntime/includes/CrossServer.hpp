#include "Helpers/TCPConnection.hpp"

#include "Helpers/Allocation.hpp"

class CrossServerConnection {
    public:

};

void CrossServerThreadFunction(Core::unique_ptr<Core::Network::TCPClient> server_sock);