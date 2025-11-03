#include "Minecraft/Hooks/MainMenuLayoutLoad.hpp"

#include <vector>

#include <CTRPluginFramework.hpp>

#define JSON_NO_IO
#define JSON_NOEXCEPTION
#include "json.hpp"

using json = nlohmann::ordered_json;

#include "string_hash.hpp"

#include "Core/CrashHandler.hpp"
#include "CoreGlobals.hpp"
#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"

#include "Minecraft/game_utils/game_functions.hpp"

namespace CTRPF = CTRPluginFramework;

namespace MenuButtonID {
    typedef enum {
        Play = 0,
        Options,
        Skins, 
        Achievements,
        Manual, 
        Multiplayer,
        Store,
    } MenuButtonID;
}

typedef struct {
    char data[0xc4];
} GameButton;

typedef struct {
    char data[0x90];
} GameCharacterView;

struct ctx_info {
    void *btnPtr;
    void *ptr2;
};

typedef struct btn_ctx_struct {
    GameButton* btnPtr = NULL;
    void *ptr2 = NULL;
} btn_ctx;

typedef struct uv_vals_struct {
    int u;
    int v;
    int w;
    int h;
} uv_vals;

typedef void (*code2Func)(int *);

typedef struct {
    int x = 0, y = 0, width = 0, height = 0;
    int iconU = 0, iconV = 0, iconW = 0, iconH = 0;
    std::string text;
    bool bigIcon = false;
} MenuBtnData;

typedef struct {
    float x = 0.0f, y = 0.0f;
    int width = 0, height = 0;
} MenuChrtData;

static std::vector<MenuBtnData> MenuLayoutBtns;
static std::vector<MenuButtonID::MenuButtonID> MenuBtnsOrder;
static MenuChrtData MenuLayoutChrt;

static void CreateMenuButtons(int *ptr, std::vector<btn_ctx> &btn_ctxs) {
    GameButton *(*InitMenuButton)(GameButton*ptr1, int* ptr2, MenuButtonID::MenuButtonID submenuID, int posX, int posY, int width, int height, const char *string, int buttonType) = (GameButton*(*)(GameButton*, int*, MenuButtonID::MenuButtonID, int, int, int, int, const char *, int))(0x5d6b58);
    void (*AddButtonTexUVs)(GameButton* btnPtr, void*, int u, int v, int w, int h, int, uv_vals*, uv_vals*, int, int, int) = (void(*)(GameButton*, void*, int, int, int, int, int, uv_vals*, uv_vals*, int, int, int))(0x5d6a50);

    // Button bg uvs
    uv_vals uv1 = {0x38, 0x90, 8, 8};
    uv_vals uv2 = {0x40, 0x90, 8, 8};

    // --- Define all buttons ---
    for (auto &i : MenuBtnsOrder) {
        GameButton *newButton = (GameButton*)GameMemalloc(sizeof(GameButton));
        if (newButton) {
            MenuBtnData& btnData = MenuLayoutBtns[i];
            InitMenuButton(newButton, (int*)ptr[1], i, btnData.x, btnData.y, btnData.width, 
                        btnData.height, btnData.text.c_str(), btnData.bigIcon);
            // MaybeLinkButton
            reinterpret_cast<void(*)(btn_ctx&, GameButton*)>(0x8b1bc0)(btn_ctxs[i], newButton);

            AddButtonTexUVs(btn_ctxs[i].btnPtr, (void*)0xabfd74, btnData.iconU,
                        btnData.iconV, btnData.iconW, btnData.iconH,
                        0, &uv2, &uv1, 2, 2, 0);
            struct ctx_info tex_ctx;
            // MaybeLinkButtonTexs
            reinterpret_cast<void(*)(void*, btn_ctx&)>(0x8b1c74)(&tex_ctx, btn_ctxs[i]);
            // MaybeRegisterData
            reinterpret_cast<void(*)(int*, void*)>(0x8f9788)(ptr + 7, &tex_ctx);
            // MaybeAppendButton
            reinterpret_cast<void(*)(void*)>(0x8b1074)(&tex_ctx);
        }
    }   
}

static void CreateMainMenuCustomLayout(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    void (*InitMenuCharacter)(GameCharacterView* chrPtr,int*,float,float,int,int,int,int) = (void(*)(GameCharacterView*,int*,float,float,int,int,int,int))(0x1ec930);
    void (*CreateUpdatePopUp)(int*,int,int,int) = (void(*)(int*,int,int,int))(0x26eb1c);

    *(char *)0xa35877 = 1;
    std::vector<btn_ctx> btn_ctxs(7);
    CreateMenuButtons(ptr, btn_ctxs);

    // --- Character container ---
    GameCharacterView *dataPtr = (GameCharacterView*)GameMemalloc(sizeof(GameCharacterView));
    if (dataPtr != NULL) {
        InitMenuCharacter(dataPtr, (int*)ptr[1], MenuLayoutChrt.x, MenuLayoutChrt.y, 
                        0x32, 0x50, 2, 1);
    }

    ptr[0x24] = (int)dataPtr;
    CreateUpdatePopUp(ptr, ptr[1], 0xf0, 0x90); // Pop-up message

    // --- Add buttons border selection ---
    if (*(char*)(ptr + 0x29) == 0) {
        for (u32 i = 0; i < (u32)(ptr[8] - ptr[7]) / 8; i++) {
            // MaybeRegisterData
            reinterpret_cast<void(*)(int*, void*)>(0x8f9788)(ptr + 0xd, (void*)(ptr[7] + i * 8));
        }
        // UnknownFunctionality
        reinterpret_cast<void(*)(int*,int)>(0x61f300)(ptr, 0);
        code2Func *code2 = (code2Func *)(*ptr + 0x174);
        code2[0](ptr);
    }
    
    for (auto i = MenuBtnsOrder.rbegin(); i != MenuBtnsOrder.rend(); i++) {
        // MaybeUpdateData
        reinterpret_cast<btn_ctx*(*)(btn_ctx&)>(0x8b1c18)(btn_ctxs[*i]);
    }
    
    Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;
    GameState.MainMenuLoaded.store(true);
    return;
}

// Note: If a layout is loaded this will overwrite callback
// So always do the same as the callback in MainMenuLayoutLoadCallback
void PatchGameMenuLayoutFunction() {
    CTRPF::Process::Write32(0x9ab4a4, (u32)CreateMainMenuCustomLayout); // Patch only reference to CreateMenuButtons
}

static void MainMenuLayoutLoadCallback(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    // MainMenuLoadOriginal
    reinterpret_cast<void(*)(int*)>(0x26eda4)(ptr);

    Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;
    GameState.MainMenuLoaded.store(true);
    return;
}

void SetMainMenuLayoutLoadCallback() {
    CTRPF::Process::Write32(0x9ab4a4, (u32)MainMenuLayoutLoadCallback); // Patch only reference to CreateMenuButtons
}

static void LoadButtonData(json &j, MenuBtnData &btnData) {
    btnData.x = j.value("x", 0);
    btnData.y = j.value("y", 0);
    btnData.width = j.value("width", 0);
    btnData.height = j.value("height", 0);
    btnData.text = j.value("text", "");
    btnData.bigIcon = j.value("bigIcon", false);
    if (j.contains("icon") && j["icon"].is_array() && j["icon"].size() == 4) {
        json &icon = j["icon"];
        if (icon[0].is_number())
            btnData.iconU = icon[0];
        if (icon[1].is_number())
            btnData.iconV = icon[1];
        if (icon[2].is_number())
            btnData.iconW = icon[2];
        if (icon[3].is_number())
            btnData.iconH = icon[3];
    }
    if (btnData.text.find("%d.%d.%d") != std::string::npos) 
        btnData.text = CTRPF::Utils::Format(btnData.text.c_str(), PLG_VER_MAJ, PLG_VER_MIN, PLG_VER_PAT);
}

bool LoadGameMenuLayout(const std::string& filepath) {
    Core::Debug::LogMessage("Loading menu layout: '" + filepath + "'", false);
    std::string fileCont = Core::Utils::LoadFile(filepath);
    if (!fileCont.empty()) {
        json j = json::parse(std::string(fileCont), nullptr, false);
        if (!j.is_discarded()) {
            if (j.contains("buttons") && j["buttons"].is_object()) {
                MenuBtnsOrder.clear();
                MenuLayoutBtns.resize(7);
                json& btnsData = j["buttons"];
                for (auto& [key, value] : btnsData.items()) {
                    if (value.is_object()) {
                        u32 key_hash = hash(key.c_str());
                        bool valid = true;
                        MenuBtnData* btnData = &MenuLayoutBtns[MenuButtonID::Play];
                        switch (key_hash) {
                            case hash("play"): 
                                MenuBtnsOrder.push_back(MenuButtonID::Play);
                                btnData = &MenuLayoutBtns[MenuButtonID::Play];
                                break;
                            case hash("multiplayer"):
                                MenuBtnsOrder.push_back(MenuButtonID::Multiplayer);
                                btnData = &MenuLayoutBtns[MenuButtonID::Multiplayer];
                                break;
                            case hash("options"):
                                MenuBtnsOrder.push_back(MenuButtonID::Options);
                                btnData = &MenuLayoutBtns[MenuButtonID::Options];
                                break;
                            case hash("skins"):
                                MenuBtnsOrder.push_back(MenuButtonID::Skins);
                                btnData = &MenuLayoutBtns[MenuButtonID::Skins];
                                break;
                            case hash("achievements"):
                                MenuBtnsOrder.push_back(MenuButtonID::Achievements);
                                btnData = &MenuLayoutBtns[MenuButtonID::Achievements];
                                break;
                            case hash("manual"):
                                MenuBtnsOrder.push_back(MenuButtonID::Manual);
                                btnData = &MenuLayoutBtns[MenuButtonID::Manual];
                                break;
                            case hash("store"):
                                MenuBtnsOrder.push_back(MenuButtonID::Store);
                                btnData = &MenuLayoutBtns[MenuButtonID::Store];
                                break;
                            default:
                                valid = false;
                                break;
                        }
                        if (valid) {
                            LoadButtonData(value, *btnData);
                        }
                    }
                }

                if (j.contains("character") && j["character"].is_object()) {
                    json& chrtData = j["character"];
                    if (chrtData.contains("x") && chrtData["x"].is_number())
                        MenuLayoutChrt.x = chrtData["x"];
                    if (chrtData.contains("y") && chrtData["y"].is_number())
                        MenuLayoutChrt.y = chrtData["y"];
                    return true;
                }
            }
        }
    }
    Core::Debug::LogMessage("Failed to load menu layout", true);
    return false;
}