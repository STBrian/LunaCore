#pragma once

#include "TextLabel.hpp"

namespace RenderAPI {
    extern C2D_Font defaultFont;
}

class MessageBox : public BaseComponent {
    private:
    int mX, mY, mWidth, mHeight;
    TextLabel mMessageLabel;

    public:
    MessageBox() :
    mMessageLabel(RenderAPI::defaultFont, "Placeholder") {

    }

    void draw() {
        this->mMessageLabel.draw();
    }
};