#pragma once

#include <citro2d.h>

#include "ScreenStack.hpp"

class Renderer {
    private:
    static bool gfxInitializated;

    bool mOwnsBackend = false;
    C3D_RenderTarget* mTopScreen;
    C3D_RenderTarget* mBottomScreen;
    ScreenStack mStack;

    public:
    Renderer() {}

    void init();
    void fini();

    void draw(ScreenID screen);

    ScreenStack& getScreenStack() {
        return this->mStack;
    }
};