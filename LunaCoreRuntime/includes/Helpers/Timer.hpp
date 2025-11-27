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

    void Restart() {
        _clock.Restart();
    }

    CTRPluginFramework::Time GetRemainingTime() {
        return Expired() ? CTRPluginFramework::Ticks(0) : _time - _clock.GetElapsedTime();
    }

    private:
    CTRPluginFramework::Time _time;
    CTRPluginFramework::Clock _clock;
};
}