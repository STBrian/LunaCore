#pragma once

#include <3ds/types.h>

class BaseComponent {
    protected:
    u32 mX, mY;

    public:
    BaseComponent() {}

    virtual ~BaseComponent() = default;

    virtual void update() {};
    virtual void draw() = 0;
};