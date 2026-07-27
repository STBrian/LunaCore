#include "CrossServer.hpp"

#include "MC3DSPluginFramework.hpp"

#include "Helpers/Allocation.hpp"

namespace MC3DSPF = MC3DSPluginFramework;
using MC3DSApi = MC3DSPluginFramework::Facade;

void CrossServerThreadFunction(Core::unique_ptr<Core::Network::TCPClient> server_sock) {
    while (MC3DSApi::isInLevel()) {
        server_sock->SendAll("hola", 5);
    }
}