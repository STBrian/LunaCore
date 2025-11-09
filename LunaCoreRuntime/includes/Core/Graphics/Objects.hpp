#pragma once

#include "Core/Graphics/Drawable.hpp"

namespace Core {
class Rect : public Drawable {
    public:
    short width, height;
    bool filled;

    Rect(short x, short y, short width, short height, bool filled) {
        this->id = DrawableType::Rect;
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->filled = filled;
    }

    void draw(const CTRPluginFramework::Screen& screen) override {
        screen.DrawRect(this->x, this->y, this->width, this->height, this->color, this->filled);
    }
};

class Label : public Drawable {
    public:
    bool useSystemFont;
    CTRPluginFramework::Color bgColor = CTRPluginFramework::Color::Black;
    private:
    std::string text = "";

    public:
    Label(short x, short y, bool useSystemFont) {
        this->id = DrawableType::Label;
        this->x = x;
        this->y = y;
        this->useSystemFont = useSystemFont;
        this->color = CTRPluginFramework::Color::White;
    }

    void setText(const std::string& text) {
        this->text = text;
    }

    void setBgColor(const CTRPluginFramework::Color& color) {
        this->bgColor = color;
    }

    void setFgColor(const CTRPluginFramework::Color& color) {
        this->color = color;
    }

    void draw(const CTRPluginFramework::Screen& screen) override {
        if (this->useSystemFont)
            screen.DrawSysfont(this->text, this->x, this->y, this->color);
        else
            screen.Draw(this->text, this->x, this->y, this->color, this->bgColor);
    }
};
}