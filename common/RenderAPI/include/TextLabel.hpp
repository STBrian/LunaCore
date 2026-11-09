#pragma once

#include <string.h>
#include <string>
#include <memory>

#include <citro2d.h>
#include "BaseComponent.hpp"
#include "Vector2.hpp"

struct TextLabel_C2D_TextBuf_Deleter {
    void operator()(C2D_TextBuf buffer) const noexcept {
        if (buffer) {
            C2D_TextBufDelete(buffer);
        }
    }
};

class TextLabel : public BaseComponent {
    private: 
        std::string text;
        std::unique_ptr<C2D_TextBuf_s, TextLabel_C2D_TextBuf_Deleter> textBuffer;
        C2D_Text textObject;
        u32 color;
        C2D_Font font;
        float wrap;
        int align;
        bool parsed;
        float x;
        float y;
        float scaleX;
        float scaleY;
        bool matchBaselineV;
        bool show;
        
    public:
        TextLabel() = delete;
        TextLabel(const TextLabel&) = delete;
        TextLabel& operator=(const TextLabel&) = delete;

        TextLabel(TextLabel&&) = default;
        TextLabel& operator=(TextLabel&&) = default;

        ~TextLabel() {}
        
        TextLabel(C2D_Font font, const std::string &text) {
            this->font = font;
            this->text = text;
            this->textBuffer = nullptr;
            this->parsed = false;
            this->color = C2D_Color32(0, 0, 0, 255);
            this->x = this->y = 0;
            this->scaleX = this->scaleY = 1;
            this->wrap = 0;
            this->align = C2D_AlignLeft;
            this->matchBaselineV = false;
            this->show = true;
            this->set(text);
        }


    public:
        void set(const std::string &new_text) {
            if (new_text != this->text || !this->parsed) {
                this->parsed = false;
                if (new_text.size() != this->text.size() && this->textBuffer) {
                    this->textBuffer.reset(nullptr);
                }

                if (this->textBuffer.get())
                    C2D_TextBufClear(this->textBuffer.get());
                else
                    this->textBuffer.reset(C2D_TextBufNew(new_text.size() + 1));

                if (this->textBuffer) {
                    const char* success = C2D_TextFontParse(&this->textObject, this->font, this->textBuffer.get(), new_text.c_str());
                    if (success != NULL && *success == '\0') {
                        this->parsed = true;
                        C2D_TextOptimize(&this->textObject);
                        this->text = new_text;
                    }
                }
            }
        }

        void atBaseline(bool value) {
            this->matchBaselineV = value;
        }

        void setFont(C2D_Font font) {
            this->font = font;
            this->parsed = false;
            this->set(this->text);
        }

        void setColor(uint32_t color) {
            this->color = color;
        }

        void setPosition(float x, float y) {
            this->x = x;
            this->y = y;
        }

        void setScale(float scaleX, float scaleY) {
            this->scaleX = scaleX;
            this->scaleY = scaleY;
        }

        void setWrap(float wrap) {
            this->wrap = wrap;
        }

        void setAlign(int align) {
            this->align = align;
        }

        Vector2 getDimensions() {
            if (this->parsed) {
                float outWidth, outHeight;
                C2D_TextGetDimensions(&this->textObject, this->scaleX, this->scaleY, &outWidth, &outHeight);
                return Vector2{outWidth, outHeight};
            }
            else
                return Vector2{0.0f, 0.0f};
        }

        //Vector2 getDimensionsWithWrap();

        void draw() override {
            if (this->show && this->parsed) {
                u32 drawFlags = C2D_WithColor;
                if (this->matchBaselineV)
                    drawFlags |= C2D_AtBaseline;
                if (this->wrap > 0)
                    drawFlags |= C2D_WordWrap;
                drawFlags |= this->align;

                if (this->wrap > 0)
                    C2D_DrawText(&this->textObject, drawFlags, this->x, this->y, 0, this->scaleX, this->scaleY, this->color, this->wrap);
                else
                    C2D_DrawText(&this->textObject, drawFlags, this->x, this->y, 0, this->scaleX, this->scaleY, this->color);
            }
        }
};