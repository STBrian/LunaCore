#pragma once

#include "BaseComponent.hpp"

class ProgressBar : public BaseComponent {
    private:
    u32 mWidth, mHeight;
    u32 mColor;
    u32 mBgColor;
    u32 mBorderWidth;
    u32 mBorderColor;
    float mPercentage;

    public:
    ProgressBar(u32 x, u32 y, u32 width, u32 height)
    {
        this->mX = x;
        this->mY = y;
        this->mWidth = width;
        this->mHeight = height;
        this->mBgColor = 0xFF000000;
        this->mColor = 0xFFFFFFFF;

        this->mBorderColor = 0xFF000000;
        this->mBorderWidth = 2;

        this->mPercentage = 0;
    }

    void setColor(u32 color) {
        this->mColor = color;
    }

    void setBgColor(u32 color) {
        this->mBgColor = color;
    }

    void setBorderWidth(u32 width) {
        this->mBorderWidth = width;
    }

    void setBorderColor(u32 color) {
        this->mBorderColor = color;
    }

    void setProgress(float p) {
        this->mPercentage = p;
    }

    void draw() override;
};