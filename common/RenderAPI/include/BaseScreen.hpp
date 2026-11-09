#pragma once

#include "Screens.hpp"

class BaseScreen {
    private:

    public:
    BaseScreen() {}

    virtual ~BaseScreen() = default;

    virtual void init() {};
    virtual void update() {};
    virtual void draw(ScreenID screen) = 0;
};