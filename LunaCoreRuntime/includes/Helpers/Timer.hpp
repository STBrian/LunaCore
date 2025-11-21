#pragma once

#include <CTRPluginFramework.hpp>

namespace Core {
class Timer {
    public:
    Timer(const CTRPluginFramework::Time& time) {
        _time = time;
    }

    bool Expired() {
        return _clock.HasTimePassed(_time);
    }

    private:
    CTRPluginFramework::Time _time;
    CTRPluginFramework::Clock _clock;
};
}