#include "Renderer.hpp"

#include <3ds.h>

bool Renderer::gfxInitializated = false;

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static bool IsEmulator(void) {
    s64 output = 0;
    svcGetSystemInfo(&output, 0x20000, 0);
    return output;
}

void Renderer::init() 
{
    if (!Renderer::gfxInitializated) {
        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

        u8 isNot2DS = false;
        CFGU_GetModelNintendo2DS(&isNot2DS);
        if (isNot2DS && !IsEmulator()) { 
            gfxSetWide(true);
            this->mTopScreen = C3D_RenderTargetCreate(240, 800, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
            C3D_RenderTargetSetOutput(this->mTopScreen, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
        } else {
            this->mTopScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        }
        this->mBottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
        C2D_Prepare();

        this->mOwnsBackend = true;
        Renderer::gfxInitializated = true;
    }
}

void Renderer::fini() 
{
    if (this->mOwnsBackend && Renderer::gfxInitializated) {
        C2D_Fini();
        C3D_Fini();
        Renderer::gfxInitializated = false;
    }
}

void Renderer::draw(ScreenID screen)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(this->mTopScreen, 0);
    C2D_TargetClear(this->mBottomScreen, 0);

    if (screen == ScreenID::TOP || screen == ScreenID::BOTH) {
        C2D_SceneBegin(this->mTopScreen);
        this->mStack.drawScreens(ScreenID::TOP);
    }

    if (screen == ScreenID::BOTTOM || screen == ScreenID::BOTH) {
        C2D_SceneBegin(this->mBottomScreen);
        this->mStack.drawScreens(ScreenID::BOTTOM);
    }

    C3D_FrameEnd(0);
}