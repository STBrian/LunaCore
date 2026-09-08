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
#include "TextLabel.hpp"

#include "tex3dst.h"

extern u32 __ctru_heap_size;

static u32 g_MemoryUsage = 0;

extern "C" {
u32 malloc_usable_size(void*);
void* __real_malloc(size_t);
void* __real_calloc(size_t, size_t);
void* __real_realloc(void*, size_t);
void __real_free(void*);

void* __wrap_malloc(size_t size) {
    void* p = __real_malloc(size);
    if (p)
        g_MemoryUsage += malloc_usable_size(p);
    return p;
}

void* __wrap_calloc(size_t n, size_t s) {
    void* p = __real_calloc(n, s);
    if (p)
        g_MemoryUsage += malloc_usable_size(p);
    return p;
}

void* __wrap_realloc(void* p, size_t ns) {
    u32 osize = 0;
    if (p) {
        osize = malloc_usable_size(p);
    }
    void* np = __real_realloc(p, ns);
    if (np) {
        g_MemoryUsage += malloc_usable_size(np) - osize;
    }
    return np;
}

void __wrap_free(void* p) {
    if (p)
        g_MemoryUsage -= malloc_usable_size(p);
    __real_free(p);
}
}

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
C2D_Font g_GlobalFont;

class BackgroundScreen : public BaseScreen {
    private:
    static constexpr u32 WHITE = C2D_Color32(255, 255, 255, 255);

    private:
    u32 mLastState;
    std::unique_ptr<ProgressBar> mPrimaryBar;
    std::unique_ptr<ProgressBar> mSecondaryBar;
    std::unique_ptr<TextLabel> mMemoryUsageLabel;


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

        mMemoryUsageLabel = std::make_unique<TextLabel>(g_GlobalFont, "Memory: 0/0");
        Vector2 dims = mMemoryUsageLabel->getDimensions();
        mMemoryUsageLabel->setPosition(395 - dims.x, 5);
        mMemoryUsageLabel->setScale(0.4, 0.4);
    }

    void update() override {
        if (g_ProgressDisplayInfo.state != this->mLastState) {
            this->mPrimaryBar->setProgress((float)g_ProgressDisplayInfo.curStepPrimary / g_ProgressDisplayInfo.maxStepsPrimary);
            this->mSecondaryBar->setProgress((float)g_ProgressDisplayInfo.curStepSecondary / g_ProgressDisplayInfo.maxStepsSecondary);
        }
        char buffer[0x40];
        snprintf(buffer, sizeof(buffer), "Memory: %lu/%lu", g_MemoryUsage, __ctru_heap_size);
        mMemoryUsageLabel->set(buffer);
        Vector2 dims = mMemoryUsageLabel->getDimensions();
        mMemoryUsageLabel->setPosition(395 - dims.x, 5);
    }

    void draw(ScreenID screen) override {
        if (screen == ScreenID::TOP) {
            C2D_DrawRectSolid(0, 0, 0, 400, 240, WHITE);
            mPrimaryBar->draw();
            mSecondaryBar->draw();
            mMemoryUsageLabel->draw();
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
#define ASSETS_FOLDER LUNACORE_FOLDER "/data/assets"

char lfsDirectory[] = "sdmc:/luma/titles/0004000000000000/romfs";
#define PATH_LOW_TITLE_POS 26

struct GameInfo {
    u32 lowTitleId;
    FS_MediaType titleMt;
};

GameInfo g_GameInfo{0};
const char* g_LastErrMsg = nullptr;

bool isCitra(void) {
    s64 output = 0;
    svcGetSystemInfo(&output, 0x20000, 0);
    return output;
}

#define DEBUG

void BuilderMainThread(void* arg) {
    namespace fs = std::filesystem;
    if (!fs::exists(MODS_FOLDER) || !fs::is_directory(MODS_FOLDER)) {
        printf("No mods found\n");
        threadExit(0);
    }

    #ifdef DEBUG
    svcSleepThread(1000000000ULL);
    #endif
    
    fs::directory_iterator modsDir(MODS_FOLDER);

    std::vector<fs::path> modsPaths;
    for (auto& dir : modsDir) {
        fs::path modDir(dir.path());
        fs::path modManifest = modDir / "mod.json";
        if (dir.is_directory() && fs::exists(modManifest) && fs::is_regular_file(modManifest))
            modsPaths.push_back(modDir);
    }

    fs::path atlasFolder = ASSETS_FOLDER "/atlas";

    g_ProgressDisplayInfo.configPrimary(3);

    /* --- Items atlas --- */
    // Check for resources
    if (!fs::is_regular_file(atlasFolder / "atlas.items.vanilla.3dst") || !fs::is_regular_file(atlasFolder / "atlas.items.vanilla.uvs")) {
        printf("Error: Skipping building resources for item textures because the necessary resources aren't present\n");
    } else {
        // Check for item textures
        std::vector<fs::path> entries;
        for (auto& mod : modsPaths) {
            fs::path modItemsPath = mod / "assets" / "textures" / "items";
            if (fs::is_directory(modItemsPath)) {
                entries.push_back(modItemsPath);
            }
        }

        g_ProgressDisplayInfo.configSecondary(entries.size());
        printf("Opening texture: atlas.items.vanilla.3dst\n");
        Texture3dst atlas{};
        atlas.open((atlasFolder / "atlas.items.vanilla.3dst"));
        if (atlas.texstate != Texture3dst::Tex3dstState::SUCCESS) {
            printf("Error: Skipping building resources for item textures because one or more resources couldn't be loaded\n");
        } else {
            for (auto& path : entries) {
                g_ProgressDisplayInfo.updateSecondary();
            }
        }

    }
    g_ProgressDisplayInfo.updatePrimary();


    threadExit(0);
}

int main(int argc, const char** argv) {
    Result res;
    bool exit = false;
    bool unsupported = false;
    bool romfsMountedState = false;

    if (argc < 3) {
    #ifndef DEBUG
        return 1;
    #else
        g_GameInfo.lowTitleId = 0x1b8700;
        g_GameInfo.titleMt = FS_MediaType::MEDIATYPE_SD;
    #endif
    } else {
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
    }

    cfguInit();
    res = romfsInit();
    romfsMountedState = R_SUCCEEDED(res);
    
    u8 model = 0;
    CFGU_GetSystemModel(&model);
    if (model < CFG_MODEL_N3DS || model == CFG_MODEL_2DS)
        unsupported = true;

    u8 currentRegion;
    CFGU_SecureInfoGetRegion(&currentRegion);

    if (unsupported) exit = true;
    else if (!romfsMountedState) exit = true;

    if (!exit) {
        gfxInitDefault();
        std::srand(std::time(nullptr));

        Renderer render{};
        render.init();

        g_GlobalFont = C2D_FontLoadSystem((CFG_Region)currentRegion);

        sceneInit(render);
        consoleInit(gfxScreen_t::GFX_BOTTOM, nullptr);

        s32 prio;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        Thread bmainThreadHandle = threadCreate(BuilderMainThread, nullptr, 0x4000, prio + 1, -2, false);

        bool threadFinished = false;
        bool launchGame = false;
        while (aptMainLoop()) 
        {
            hidScanInput();

            u32 kDown = hidKeysDown();
            u32 kHeld = hidKeysHeld();
            if (kDown & KEY_START)
                break;

            if (!threadFinished) {
                res = threadJoin(bmainThreadHandle, 1000);
                if (R_SUCCEEDED(res)) {
                    threadFinished = true;
                    int exitCode = threadGetExitCode(bmainThreadHandle);
                    if (exitCode == 0) {
                        launchGame = true;
                        printf("Thread exited successfully\n");
                    } else {
                        printf("Thread exited with code: %d\n", exitCode);
                        printf("Error message: %s\n", g_LastErrMsg ? g_LastErrMsg : "No error message found");
                    }
                }
            }
            #ifdef DEBUG
            if (threadFinished && (kDown & KEY_SELECT))
            #else
            if (threadFinished) 
            #endif
            {
                threadFree(bmainThreadHandle);
                break;
            }

            render.getScreenStack().update();
            render.draw(ScreenID::TOP);
        }

        if (launchGame && !isCitra())
            scheduleNextApp(g_GameInfo.lowTitleId, g_GameInfo.titleMt);

        render.fini();
        gfxExit();
    }

    cfguExit();
    romfsExit();
    return 0;
}