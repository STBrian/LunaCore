#include <3ds.h>

#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <cerrno>

#include "BaseScreen.hpp"
#include "ProgressBar.hpp"
#include "Renderer.hpp"

#include "tex3dst.h"

struct ProgressInfo {
    u32 maxStepsPrimary;
    u32 curStepPrimary;
    u32 maxStepsSecondary;
    u32 curStepSecondary;

    u32 state;

    void configPrimary(u32 maxSteps) {
        this->maxStepsPrimary = maxSteps;
        this->curStepPrimary = 0;
        this->state++;
    }

    void configSecondary(u32 maxSteps) {
        this->maxStepsSecondary = maxSteps;
        this->curStepSecondary = 0;
        this->state++;
    }

    void updatePrimary() {
        this->curStepPrimary++;
        this->state++;
    }

    void updateSecondary() {
        this->curStepSecondary++;
        this->state++;
    }
};

ProgressInfo g_ProgressDisplayInfo{0};

class BackgroundScreen : public BaseScreen {
    private:
    static constexpr u32 WHITE = C2D_Color32(255, 255, 255, 255);

    private:
    u32 mLastState;
    std::unique_ptr<ProgressBar> mPrimaryBar;
    std::unique_ptr<ProgressBar> mSecondaryBar;

    public:
    BackgroundScreen() {}

    void init() override {
        mPrimaryBar = std::make_unique<ProgressBar>(20, 200, 360, 20);
        mPrimaryBar->setBorderWidth(2);
        mPrimaryBar->setColor(C2D_Color32(241, 50, 62, 255));
        mPrimaryBar->setBgColor(WHITE);
        mSecondaryBar = std::make_unique<ProgressBar>(20, 160, 360, 20);
        mSecondaryBar->setBorderWidth(2);
        mSecondaryBar->setColor(C2D_Color32(241, 50, 62, 255));
        mSecondaryBar->setBgColor(WHITE);
        this->mLastState = g_ProgressDisplayInfo.state;
    }

    void update() override {
        if (g_ProgressDisplayInfo.state != this->mLastState) {
            this->mPrimaryBar->setProgress(g_ProgressDisplayInfo.curStepPrimary / g_ProgressDisplayInfo.maxStepsPrimary);
            this->mSecondaryBar->setProgress(g_ProgressDisplayInfo.curStepSecondary / g_ProgressDisplayInfo.maxStepsSecondary);
        }
    }

    void draw(ScreenID screen) override {
        if (screen == ScreenID::TOP) {
            C2D_DrawRectSolid(0, 0, 0, 400, 240, WHITE);
            mPrimaryBar->draw();
            mSecondaryBar->draw();
        }
    }
};

void sceneInit(Renderer& render) {
    std::unique_ptr<BackgroundScreen> background(std::make_unique<BackgroundScreen>());
    render.getScreenStack().pushScreen(std::move(background));
}

void scheduleNextApp(u32 lowId, u8 md) {
    aptSetChainloader(((u64)(0x00040000) << 32) | (u64)lowId, md);
}

#define LUNACORE_FOLDER "sdmc:/Minecraft 3DS"
#define MODS_FOLDER LUNACORE_FOLDER "/mods"
#define ASSETS_FOLDER LUNACORE_FOLDER "/assets"

char lfsDirectory[] = "sdmc:/luma/titles/0004000000000000/romfs";
#define PATH_LOW_TITLE_POS 26

struct GameInfo {
    u32 lowTitleId;
    FS_MediaType titleMt;
};

GameInfo g_GameInfo{0};

void BuilderMainThread(void* arg) {
    namespace fs = std::filesystem;
    if (!fs::exists(MODS_FOLDER) || !fs::is_directory(MODS_FOLDER)) threadExit(1);
    
    fs::directory_iterator modsDir(MODS_FOLDER);

    std::vector<fs::path> modsPaths;
    for (auto& dir : modsDir) {
        fs::path modDir(dir.path());
        fs::path modManifest = modDir / "mod.json";
        if (dir.is_directory() && fs::exists(modManifest) && fs::is_regular_file(modManifest))
            modsPaths.push_back(modDir);
    }

    std::vector<fs::path> curEntries;

    // Items atlas
    for (auto& mod : modsPaths) {
        fs::path modItemsTexsPath = mod / "assets" / "textures" / "items";
    }

    threadExit(0);
}

int main(int argc, const char** argv)
{
    Result res;
    bool exit = false;
    bool unsupported = false;
    bool romfsMountedState = false;

    if (argc < 3) return 1;
    char* endPos;
    errno = 0;
    u32 retLowTitleId = strtoul(argv[1], &endPos, 16);
    FS_MediaType retMt = FS_MediaType::MEDIATYPE_SD;
    if (strcmp(argv[2], "card") == 0)
        retMt = FS_MediaType::MEDIATYPE_GAME_CARD;
    else if (strcmp(argv[2], "nand") == 0)
        retMt = FS_MediaType::MEDIATYPE_NAND;
    memcpy(lfsDirectory + PATH_LOW_TITLE_POS, argv[1], 8);
    g_GameInfo.lowTitleId = retLowTitleId;
    g_GameInfo.titleMt = retMt;

    cfguInit();
    res = romfsInit();
    romfsMountedState = R_SUCCEEDED(res);
    
    u8 model = 0;
    CFGU_GetSystemModel(&model);
    if (model < CFG_MODEL_N3DS || model == CFG_MODEL_2DS)
        unsupported = true;

    if (unsupported) exit = true;
    else if (!romfsMountedState) exit = true;

    if (!exit) {
        gfxInitDefault();
        std::srand(std::time(nullptr));

        Renderer render{};
        render.init();

        sceneInit(render);
        consoleInit(gfxScreen_t::GFX_BOTTOM, nullptr);

        s32 prio;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        Thread bmainThreadHandle = threadCreate(BuilderMainThread, nullptr, 0x4000, prio + 1, -2, false);

        bool firstThreadExit = true;
        bool launchGame = false;
        while (aptMainLoop()) 
        {
            hidScanInput();

            u32 kDown = hidKeysDown();
            u32 kHeld = hidKeysHeld();
            if (kDown & KEY_START)
                break;

            res = threadJoin(bmainThreadHandle, 1000);
            if (firstThreadExit && R_SUCCEEDED(res)) {
                firstThreadExit = false;
                int exitCode = threadGetExitCode(bmainThreadHandle);
                printf("Thread exited with code: %d\n", exitCode);
                if (exitCode == 0) 
                    launchGame = false;
            }
            if (R_SUCCEEDED(res) && (kDown & KEY_SELECT)) {
                threadFree(bmainThreadHandle);
                break;
            }

            render.getScreenStack().update();
            render.draw(ScreenID::TOP);
        }

        if (launchGame)
            scheduleNextApp(g_GameInfo.lowTitleId, g_GameInfo.titleMt);

        render.fini();
        gfxExit();
    }

    cfguExit();
    romfsExit();
    return 0;
}