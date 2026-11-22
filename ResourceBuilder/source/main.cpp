#include <3ds.h>

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <cerrno>
#include <fstream>
#include <algorithm>
#include <cctype>

#include "BaseScreen.hpp"
#include "ProgressBar.hpp"
#include "Renderer.hpp"
#include "TextLabel.hpp"

#include "tex3dst.h"
#include "uvsParser.hpp"
#include "LangParser.hpp"
#include "CBlangParser.hpp"
#include "ModsResHash.hpp"

#include "json.hpp"

using namespace nlohmann;

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
        snprintf(buffer, sizeof(buffer), "Memory: %lu/%lu", (g_MemoryUsage / 1024), (__ctru_heap_size / 1024));
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
#define DATA_FOLDER LUNACORE_FOLDER "/data"
#define ASSETS_FOLDER DATA_FOLDER "/assets"

char lfsDirectory[] = "sdmc:/luma/titles/0004000000000000/romfs";
#define PATH_LOW_TITLE_POS 26

struct GameInfo {
    u32 lowTitleId;
    FS_MediaType titleMt;
};

const char* g_LastErrMsg = nullptr;

bool isCitra(void) {
    s64 output = 0;
    svcGetSystemInfo(&output, 0x20000, 0);
    return output;
}

bool findFreeUVSpace(UVSParser::UVValues& out, const UVSParser& parser, u32 atlasWidth, u32 atlasHeight, u32 texMaxWidth, u32 texMaxHeight) {
    s32 curX = atlasWidth - texMaxWidth, curY = atlasHeight - texMaxHeight;
    while (parser.checkCollision(UVSParser::createUVBox(atlasWidth, atlasHeight, curX, curY, curX + texMaxWidth, curY + texMaxHeight))) {
        curX -= 8;
        if (curX < 0) {
            curX = atlasWidth - texMaxWidth;
            curY -= 8;
        }
        if (curY < 0) break;
    }
    if (curY < 0) return false;
    out = UVSParser::createUVBox(atlasWidth, atlasHeight, curX, curY, curX + texMaxWidth, curY + texMaxHeight);
    return true;
}

#define DEBUG 1

int BuilderMainThread(void* arg) {
    namespace fs = std::filesystem;
    if (!fs::exists(MODS_FOLDER) || !fs::is_directory(MODS_FOLDER)) {
        printf("No mods found\n");
        return 0;
    }

    // #ifdef DEBUG
    svcSleepThread(1000000000ULL);
    // #endif
    
    ModResourcesHashDB resDb{};
    fs::directory_iterator modsDir(MODS_FOLDER);

    std::unordered_map<std::string, fs::path> modsPaths;
    printf("Checking mods\n");
    for (auto& dir : modsDir) {
        fs::path modDir(dir.path());
        fs::path modManifest = modDir / "mod.json";
        if (dir.is_directory() && fs::exists(modManifest) && fs::is_regular_file(modManifest)) {
            // Validate manifest
            std::ifstream manifestFile(modManifest);
            json j = json::parse(manifestFile, nullptr, false, false, true);
            if (j.is_discarded()) continue;

            if (!j.contains("name") || !j["name"].is_string()) continue;

            std::string name = j["name"];
            std::string verStr = "0.0.0";
            if (j.contains("version") && j["version"].is_string()) {
                verStr = j["version"];
            }
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            resDb.add((name + "@" + verStr).c_str());
            modsPaths[name] = modDir;
        }
    }

    printf("Found %u mods\n", modsPaths.size());
    if (modsPaths.empty()) return 0;

    if (fs::exists(DATA_FOLDER) && !fs::is_directory(DATA_FOLDER)) fs::remove(DATA_FOLDER);
    if (!fs::exists(DATA_FOLDER)) fs::create_directory(DATA_FOLDER);

    if (fs::exists(ASSETS_FOLDER) && !fs::is_directory(ASSETS_FOLDER)) fs::remove(ASSETS_FOLDER);
    if (!fs::exists(ASSETS_FOLDER)) fs::create_directory(ASSETS_FOLDER);

    g_ProgressDisplayInfo.configPrimary(2);

    /* --- Items atlas --- */
    {
        UVSParser uvs_256{};
        UVSParser uvs_512{};
        UVSParser uvs_1024{};

        g_ProgressDisplayInfo.configSecondary(modsPaths.size());

        printf("Building custom items uvs and textures index\n");
        for (auto& entry : modsPaths) {
            printf("Next: %s\n", entry.first.c_str());

            fs::path assetsFolder = entry.second / "assets";
            fs::path itemModelsDir = assetsFolder / "items";

            if (!fs::is_directory(itemModelsDir)) {
                g_ProgressDisplayInfo.updateSecondary();
                continue;
            }

            for (auto& modelFilePath : fs::directory_iterator(itemModelsDir)) {
                if (modelFilePath.is_regular_file() && modelFilePath.path().extension() == ".json") {
                    std::string modelName = modelFilePath.path().stem();
                    std::transform(modelName.begin(), modelName.end(), modelName.begin(), ::tolower);

                    printf("Opening item model: %s\n", modelFilePath.path().c_str());
                    std::ifstream modelFile(modelFilePath.path());
                    if (!modelFile.is_open()) {
                        printf("Failed to open: %s\n", modelFilePath.path().c_str());
                        continue;
                    }

                    printf("Parsing file...\n");
                    json j = json::parse(modelFile, nullptr, false, false, true);
                    if (j.is_discarded()) {
                        printf("Error parsing the model\n");
                        continue;
                    }

                    if (j.is_object() && j.contains("textures") && j["textures"].is_array() && j["textures"].size() > 0) {
                        std::string texNameId = "lc_" + entry.first + "_" + modelName;
                        uvs_256.addEntry(texNameId.c_str());
                        uvs_512.addEntry(texNameId.c_str());
                        printf("Added entry: %s\n", texNameId.c_str());
                        for (auto& texEntry : j["textures"]) {
                            if (texEntry.is_string()) {
                                std::string texPathId = entry.first + "/" + std::string(texEntry);
                                printf("Appending texture: %s\n", texPathId.c_str());
                                UVSParser::UVValues freeSpace;
                                if (findFreeUVSpace(freeSpace, uvs_256, 512, 256, 16, 16)) {
                                    uvs_256.append(texNameId.c_str(), texPathId.c_str(), 512, 256, freeSpace);
                                }
                                if (findFreeUVSpace(freeSpace, uvs_512, 512, 512, 16, 16)) {
                                    uvs_512.append(texNameId.c_str(), texPathId.c_str(), 512, 512, freeSpace);
                                }
                                if (findFreeUVSpace(freeSpace, uvs_1024, 1024, 512, 32, 32)) { // needs 32 textures for some reason
                                    uvs_1024.append(texNameId.c_str(), texPathId.c_str(), 1024, 512, freeSpace);
                                }
                            }
                        }
                    }
                }
            }

            g_ProgressDisplayInfo.updateSecondary();
        }
        

        printf("Saving changes to '%s'\n", ASSETS_FOLDER "/custom.items_256.uvs");
        uvs_256.dump(ASSETS_FOLDER "/custom.items_256.uvs");
        printf("Saving changes to '%s'\n", ASSETS_FOLDER "/custom.items_512.uvs");
        uvs_512.dump(ASSETS_FOLDER "/custom.items_512.uvs");
        printf("Saving changes to '%s'\n", ASSETS_FOLDER "/custom.items_1024.uvs");
        uvs_1024.dump(ASSETS_FOLDER "/custom.items_1024.uvs");
    }
    g_ProgressDisplayInfo.updatePrimary();

    /* --- LANG --- */
    {
        std::unordered_map<std::string, CBlangParser> langRegistry;

        if (fs::exists(ASSETS_FOLDER "/loc") && !fs::is_directory(ASSETS_FOLDER "/loc")) fs::remove(ASSETS_FOLDER "/loc");
        if (!fs::exists(ASSETS_FOLDER "/loc")) fs::create_directory(ASSETS_FOLDER "/loc");

        g_ProgressDisplayInfo.configSecondary(modsPaths.size());

        printf("Building custom texts\n");
        for (auto& entry : modsPaths) {
            printf("Next: %s\n", entry.first.c_str());
            fs::path assetsFolder = entry.second / "assets";
            fs::path locDir = assetsFolder / "loc";

            if (!fs::is_directory(locDir)) {
                g_ProgressDisplayInfo.updateSecondary();
                continue;
            }

            for (auto& file : fs::directory_iterator(locDir)) {
                if (file.is_regular_file() && file.path().extension() == ".lang") {
                    std::string fileName = file.path().stem();
                    std::string langNameId = fileName;
                    const std::size_t sep = fileName.find("-pocket");
                    // 0 1 2 3 4 5 6 7 8 9 10 11 12 13
                    // e n _ U S _ _ - p o c  k  e  t
                    // This should also accept names without the "-pocket" suffix
                    if ((sep != std::string::npos && (sep >= 8 || sep + 7 != fileName.size())) || (sep == std::string::npos && langNameId.size() >= 8)) continue; // invalid lang name id

                    if (sep != std::string::npos) {
                        /* remove suffix */
                        langNameId = fileName.substr(0, sep);
                    }

                    const std::size_t sep2 = langNameId.find('_');
                    if (sep2 != std::string::npos && sep2 != 0 && sep2 != langNameId.size()-1) {
                        /* Standarize name if needed */
                        std::transform(langNameId.begin(), langNameId.begin() + sep2, langNameId.begin(), ::tolower);
                        std::transform(langNameId.begin() + sep2 + 1, langNameId.end(), langNameId.begin() + sep2 + 1, ::toupper);
                    }

                    /* Add if not present already */
                    if (langRegistry.find(langNameId) == langRegistry.end()) {
                        langRegistry[langNameId] = CBlangParser{};
                    }

                    printf("Opening lang file: %s\n", file.path().c_str());
                    LangParser parser{};
                    parser.load(file.path());

                    if (!parser.empty()) {
                        for (auto& parserEntry : parser.mEntries) {
                            langRegistry[langNameId].add(parserEntry.first, parserEntry.second);
                        }
                    }
                }
            }

            g_ProgressDisplayInfo.updateSecondary();
        }

        for (auto& langRegEntry : langRegistry) {
            langRegEntry.second.dump(ASSETS_FOLDER "/loc/" + langRegEntry.first + "-pocket.cblang");
        }
    }
    g_ProgressDisplayInfo.updatePrimary();

    if (!resDb.empty()) {
        if (resDb.dump(DATA_FOLDER "/mods_res_db.bin")) {
            printf("Built '%s'\n", DATA_FOLDER "/mods_res_db.bin");
        } else {
            printf("Failed to build '%s'\n", DATA_FOLDER "/mods_res_db.bin");
        }
    }

    return 0;
}

void BuilderMainThreadWrapper(void* arg) {
    int res = BuilderMainThread(arg);
    threadExit(res);
}

int main(int argc, const char** argv) {
    Result res;
    bool exit = false;
    bool unsupported = false;
    bool romfsMountedState = false;
    bool noGameLaunch = false;
    GameInfo gameInfo{0};

    gfxInitDefault();
    hidInit();
    consoleInit(gfxScreen_t::GFX_BOTTOM, nullptr);

    if (argc < 3) {
        // Manual launch
        int presetSelected = -1;
        int presetHovered = 0;
        const char* presets[] = {
            "USA SD", "USA CARD", "USA NAND",
            "EUR SD", "EUR CARD", "EUR NAND",
            "JPN SD", "JPN CARD", "JPN NAND",
            "I don't care",
            "Exit"
        };

        const int size = sizeof(presets) / sizeof(presets[0]);
        bool needRefresh = true;

        while (presetSelected < 0) {
            if (needRefresh) {
                for (int i = 0; i < size; i++) {
                    printf("\x1b[%d;0H %s %s", i + 1, presetHovered == i ? ">" : " ", presets[i]);
                }
                needRefresh = false;
            }

            hidScanInput();

            u32 kRelease = hidKeysUp();

            if (kRelease & KEY_A) presetSelected = presetHovered;

            if (kRelease & KEY_UP) {
                if (presetHovered > 0)
                    presetHovered--;
                else 
                    presetHovered = size - 1;
                needRefresh = true;
            }

            if (kRelease & KEY_DOWN) {
                if (presetHovered < size - 1)
                    presetHovered++;
                else
                    presetHovered = 0;
                needRefresh = true;
            }
        }

        consoleClear();
        if (presetSelected == size - 1) return 0;
        else if (presetSelected == size - 2) noGameLaunch = true;
        else {
            if (presetSelected >= 6) gameInfo.lowTitleId = 0x17fd00;
            else if (presetSelected <= 2) gameInfo.lowTitleId = 0x1b8700;
            else gameInfo.lowTitleId = 0x17ca00;

            if (((presetSelected + 1) % 3) == 0) gameInfo.titleMt = FS_MediaType::MEDIATYPE_NAND;
            else if (((presetSelected + 1) % 3) == 1) gameInfo.titleMt = FS_MediaType::MEDIATYPE_SD;
            else gameInfo.titleMt = FS_MediaType::MEDIATYPE_GAME_CARD;
            printf("Selection %08lX %01X", gameInfo.lowTitleId, gameInfo.titleMt);
        }
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
        gameInfo.lowTitleId = retLowTitleId;
        gameInfo.titleMt = retMt;
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
        std::srand(std::time(nullptr));

        Renderer render{};
        render.init();

        g_GlobalFont = C2D_FontLoadSystem((CFG_Region)currentRegion);

        sceneInit(render);

        s32 prio;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        Thread bmainThreadHandle = threadCreate(BuilderMainThreadWrapper, nullptr, 0x4000, prio + 1, -2, false);

        bool threadFinished = false;
        bool launchGame = false;
        while (aptMainLoop()) 
        {
            hidScanInput();

            u32 kDown = hidKeysDown();
            u32 kHeld = hidKeysHeld();
            // if (kDown & KEY_START)
                // break;

            if (!threadFinished) {
                res = threadJoin(bmainThreadHandle, 1000);
                if (res == 0) {
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
            #if DEBUG
            if (threadFinished && (kDown & KEY_START))
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

        if (launchGame && !isCitra() && !noGameLaunch)
            scheduleNextApp(gameInfo.lowTitleId, gameInfo.titleMt);

        render.fini();
        gfxExit();
    }

    cfguExit();
    romfsExit();
    return 0;
}