#pragma once

#include <CTRPluginFramework.hpp>
#include <vector>
#include <algorithm>

#include "Core/Graphics/Objects.hpp"
#include "Helpers/Mutex.hpp"

namespace Core {
class GraphicsManager {
    private:
    std::vector<Drawable*> drawables;
    Mutex _lock;

    GraphicsManager() {
        CTRPluginFramework::OSD::Run(OSDCallback);
    }

    static bool OSDCallback(const CTRPluginFramework::Screen& screen);

    public:
    static GraphicsManager& getInstance();

    void addObject(Drawable* obj) {
        _lock.lock();
        auto it = std::find(drawables.begin(), drawables.end(), obj);
        if (it == drawables.end())
            this->drawables.push_back(obj);
        _lock.unlock();
    }

    void removeObject(Drawable* obj) {
        _lock.lock();
        auto it = std::find(drawables.begin(), drawables.end(), obj);
        if (it != drawables.end())
            drawables.erase(it);
        _lock.unlock();
    }

    void Lock() {
        _lock.lock();
    }

    void Unlock() {
        _lock.unlock();
    }

    bool TryLock() {
        return _lock.try_lock();
    }
};
}