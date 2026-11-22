#include "ProgressBar.hpp"

#include <citro2d.h>

void ProgressBar::draw() {
    C2D_DrawRectSolid(this->mX - this->mBorderWidth, this->mY - this->mBorderWidth, 0, this->mWidth + this->mBorderWidth*2, this->mHeight + this->mBorderWidth*2, this->mBorderColor);
    C2D_DrawRectSolid(this->mX, this->mY, 0, this->mWidth, this->mHeight, this->mBgColor);
    C2D_DrawRectSolid(this->mX, this->mY, 0, this->mWidth * this->mPercentage, this->mHeight, this->mColor);
}