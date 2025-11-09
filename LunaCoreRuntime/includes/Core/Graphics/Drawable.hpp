#pragma once

#include <CTRPluginFramework.hpp>

namespace Core {
class Drawable {
    public:
    enum class DrawableType {
        Base,
        Rect,
        Label
    };

    protected:
    DrawableType id = DrawableType::Base;
    public:
    bool drawTop = true;
    bool drawBottom = true;
    short x, y;
    bool visible = true;
    CTRPluginFramework::Color color = CTRPluginFramework::Color::Black;

    public:
    DrawableType getType() const { return id; };

    bool shouldDraw(const CTRPluginFramework::Screen& screen) {
        return visible && ((screen.IsTop && drawTop) || (!screen.IsTop && drawBottom));
    }

    void toggleTopScreen() {
        this->drawTop = !this->drawTop;
    }

    void toggleBottomScreen() {
        this->drawBottom = !this->drawBottom;
    }

    virtual void draw(const CTRPluginFramework::Screen& screen) = 0;

    virtual ~Drawable() = default;
};
}