#include "Core/Graphics/GraphicsManager.hpp"

namespace Core {

GraphicsManager& GraphicsManager::getInstance() {
    static GraphicsManager _singleton;
    return _singleton;
}

bool GraphicsManager::OSDCallback(const CTRPluginFramework::Screen& screen) {
    GraphicsManager& mgr = GraphicsManager::getInstance();
    mgr._lock.lock();

    if (mgr.drawables.empty()) {
        mgr._lock.unlock();
        return false;
    }

    bool frameEdited = false;

    for (auto obj : mgr.drawables) {
        if (obj->shouldDraw(screen)) {
            obj->draw(screen);
            frameEdited = true;
        }
    }

    mgr._lock.unlock();
    return frameEdited;
}

}