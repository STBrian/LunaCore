#include "Game/Hooks/MainMenuLayoutLoad.hpp"

#include <vector>

#include <CTRPluginFramework.hpp>

#define JSON_NO_IO
#define JSON_NOEXCEPTION
#include "json.hpp"

using json = nlohmann::json;

#include "Core/CrashHandler.hpp"
#include "Game/game_utils/game_functions.hpp"
#include "CoreGlobals.hpp"
#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"

#define BASE_OFF 0x100000

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
static MenuChrtData MenuLayoutChrt;

static void CreateMenuButtons(int *ptr, std::vector<btn_ctx> &btn_ctxs) {
    GameButton *(*InitMenuButton)(GameButton*ptr1, int* ptr2, MenuButtonID::MenuButtonID submenuID, int posX, int posY, int width, int height, const char *string, int buttonType) = (GameButton*(*)(GameButton*, int*, MenuButtonID::MenuButtonID, int, int, int, int, const char *, int))(0x4d6b58+BASE_OFF);
    void (*LinkButton)(btn_ctx *ptr1, GameButton *btnPtr) = (void(*)(btn_ctx*, GameButton*))(0x7b1bc0+BASE_OFF);
    void (*AddButtonTexUVs)(GameButton* btnPtr, void*, int u, int v, int w, int h, int, uv_vals*, uv_vals*, int, int, int) = (void(*)(GameButton*, void*, int, int, int, int, int, uv_vals*, uv_vals*, int, int, int))(0x4d6a50+BASE_OFF);
    void (*MaybeLinkButtonTexs)(void*, btn_ctx*) = (void(*)(void*, btn_ctx*))(0x7b1c74+BASE_OFF);
    void (*MaybeRegisterData)(int*, void*) = (void(*)(int*, void*))(0x7f9788+BASE_OFF);
    void (*MaybeAppendButton)(void*) = (void(*)(void*))(0x7b1074+BASE_OFF);

    std::vector<MenuButtonID::MenuButtonID> btnsOrder = {
        MenuButtonID::Play,
        MenuButtonID::Multiplayer,
        MenuButtonID::Options,
        MenuButtonID::Skins,
        MenuButtonID::Achievements,
        MenuButtonID::Manual,
        MenuButtonID::Store
    };

    // Button bg uvs
    uv_vals uv1 = {0x38, 0x90, 8, 8};
    uv_vals uv2 = {0x40, 0x90, 8, 8};

    // --- Define all buttons ---
    for (auto &i : btnsOrder) {
        int buttonType = 0;
        if (MenuLayoutBtns[i].bigIcon)
            buttonType = 1;
        
        GameButton *newButton = (GameButton*)GameMemalloc(sizeof(GameButton));
        if (newButton != NULL)
            InitMenuButton(newButton, (int*)ptr[1], i, MenuLayoutBtns[i].x, 
                            MenuLayoutBtns[i].y, MenuLayoutBtns[i].width, MenuLayoutBtns[i].height, 
                            MenuLayoutBtns[i].text.c_str(), buttonType);
        LinkButton(&btn_ctxs[i], newButton);

        AddButtonTexUVs(btn_ctxs[i].btnPtr,(void*)0xabfd74,MenuLayoutBtns[i].iconU,
                        MenuLayoutBtns[i].iconV,MenuLayoutBtns[i].iconW,MenuLayoutBtns[i].iconH,
                        0,&uv2,&uv1,2,2,0);
        struct ctx_info tex_ctx;
        MaybeLinkButtonTexs(&tex_ctx, &btn_ctxs[i]);
        MaybeRegisterData(ptr + 7, &tex_ctx);
        MaybeAppendButton(&tex_ctx);
    }   
}

static void CreateMainMenuCustomLayout(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    void (*MaybeRegisterData)(int*, void*) = (void(*)(int*, void*))(0x7f9788+BASE_OFF);
    btn_ctx *(*MaybeUpdateData)(btn_ctx*) = (btn_ctx*(*)(btn_ctx*))(0x7b1c18+BASE_OFF);
    u64 (*InitMenuCharacter)(GameCharacterView* chrPtr,int*,float,float,int,int,int,int) = (u64(*)(GameCharacterView*,int*,float,float,int,int,int,int))(0xec930+BASE_OFF);
    void (*CreateUpdatePopUp)(int*,int,int,int) = (void(*)(int*,int,int,int))(0x16eb1c+BASE_OFF);
    void (*UnknownFunctionality)(int*,int) = (void(*)(int*,int))(0x51f300+BASE_OFF);

    *(char *)0xa35877 = 1;
    std::vector<btn_ctx> btn_ctxs(7);
    CreateMenuButtons(ptr, btn_ctxs);

    // --- Character container ---
    GameCharacterView *dataPtr = (GameCharacterView*)GameMemalloc(sizeof(GameCharacterView));
    if (dataPtr != NULL) {
        u64 resVal = InitMenuCharacter(dataPtr, (int*)ptr[1], MenuLayoutChrt.x, MenuLayoutChrt.y, 
                                        0x32, 0x50, 2, 1);
    }

    ptr[0x24] = (int)dataPtr;
    CreateUpdatePopUp(ptr, ptr[1], 0xf0, 0x90); // Pop-up message

    // --- Add buttons border selection ---
    if (*(char*)(ptr + 0x29) == 0) {
        for (u32 i = 0; i < (u32)(ptr[8] - ptr[7]) / 8; i++) {
            MaybeRegisterData(ptr + 0xd, (void*)(ptr[7] + i * 8));
        }
        UnknownFunctionality(ptr, 0);
        code2Func *code2 = (code2Func *)(*ptr + 0x174);
        code2[0](ptr);
    }

    std::vector<MenuButtonID::MenuButtonID> btnsOrder = {
        MenuButtonID::Store,
        MenuButtonID::Manual,
        MenuButtonID::Achievements,
        MenuButtonID::Skins,
        MenuButtonID::Options,
        MenuButtonID::Multiplayer,
        MenuButtonID::Play,
    };
    
    for (auto &i : btnsOrder)
        MaybeUpdateData(&btn_ctxs[i]);
    
    Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;
    GameState.MainMenuLoaded.store(true);
    return;
}

// Note: If a layout is loaded this will overwrite callback
// So always do the same as the callback in CreateMainMenuCustomLayout
void PatchGameMenuLayoutFunction() {
    CTRPF::Process::Write32(0x8ab4a4 + BASE_OFF, (u32)CreateMainMenuCustomLayout); // Patch only reference to CreateMenuButtons
}

static void MainMenuLayoutLoadCallback(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    void (*MainMenuLoadOriginal)(int*) = (void(*)(int*))(0x16eda4+BASE_OFF);
    MainMenuLoadOriginal(ptr);

    Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;
    GameState.MainMenuLoaded.store(true);
    return;
}

void SetMainMenuLayoutLoadCallback() {
    CTRPF::Process::Write32(0x8ab4a4 + BASE_OFF, (u32)MainMenuLayoutLoadCallback); // Patch only reference to CreateMenuButtons
}

static void LoadButtonData(json &btnJData, MenuBtnData &btnData, const std::string& btnName) {
    json& j = btnJData[btnName];
    if (j.contains("x") && j["x"].is_number())
        btnData.x = j.value("x", 0);
    if (j.contains("y") && j["y"].is_number())
        btnData.y = j.value("y", 0);
    if (j.contains("width") && j["width"].is_number())
        btnData.width = j.value("width", 0);
    if (j.contains("height") && j["height"].is_number())
        btnData.height = j.value("height", 0);
    if (j.contains("text") && j["text"].is_string())
        btnData.text = j["text"].get<std::string>();
    if (j.contains("bigIcon") && j["bigIcon"].is_boolean())
        btnData.bigIcon = j.value("bigIcon", false);
    if (j.contains("icon") && j["icon"].is_array() && j["icon"].size() == 4) {
        if (j["icon"][0].is_number())
            btnData.iconU = j["icon"][0];
        if (j["icon"][1].is_number())
            btnData.iconV = j["icon"][1];
        if (j["icon"][2].is_number())
            btnData.iconW = j["icon"][2];
        if (j["icon"][3].is_number())
            btnData.iconH = j["icon"][3];
    }
}

bool LoadGameMenuLayout(const std::string& filepath) {
    std::string fileCont = Core::Utils::LoadFile(filepath);
    if (!fileCont.empty()) 
    {
        json j = json::parse(std::string(fileCont), nullptr, false);
        if (!j.is_discarded()) 
        {
            if (j.contains("buttons") && j["buttons"].is_object()) 
            {
                MenuLayoutBtns.resize(7);
                json& btnsData = j["buttons"];
                if (btnsData.contains("play") && btnsData["play"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Play], "play");
                if (btnsData.contains("multiplayer") && btnsData["multiplayer"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Multiplayer], "multiplayer");
                if (btnsData.contains("options") && btnsData["options"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Options], "options");
                if (btnsData.contains("skins") && btnsData["skins"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Skins], "skins");
                if (btnsData.contains("achievements") && btnsData["achievements"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Achievements], "achievements");
                if (btnsData.contains("manual") && btnsData["manual"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Manual], "manual");
                if (btnsData.contains("store") && btnsData["store"].is_object())
                    LoadButtonData(btnsData, MenuLayoutBtns[MenuButtonID::Store], "store");

                if (j.contains("character") && j["character"].is_object()) {
                    json& chrtData = j["character"];
                    if (chrtData.contains("x") && chrtData["x"].is_number())
                        MenuLayoutChrt.x = chrtData["x"].get<float>();
                    if (chrtData.contains("y") && chrtData["y"].is_number())
                        MenuLayoutChrt.x = chrtData["y"].get<float>();
                    return true;
                }
            }
        }
    }
    Core::Debug::LogMessage("Failed to load menu layout", true);
    return false;
}

static bool ReplaceStringWithPointer(u32 offset, u32 insAddr, u32 strAddr, u32 ptrAddr, u8 reg) {
    u32 baseIns = (0xe5bf << 16)|((reg & 0xF) << 12);
    u32 addrsOffset = strAddr - insAddr - 8;
    if (addrsOffset & 0xFFF != addrsOffset)
        return false;
    if (!CTRPF::Process::Write32(insAddr + offset, baseIns|addrsOffset)) return false; // Patch instruction
    return CTRPF::Process::Write32(strAddr + offset, ptrAddr); // Write pointer to offset
}

static bool ReplaceConstString(u32 offset, u32 insAddr, u32 strAddr, u8 reg, const std::string& text) {
    char *textPtr = (char*)GameCalloc(text.size() + 1); // Allocate string space in game memory
    if (textPtr == NULL) return false;
    if (!CTRPF::Process::WriteString((u32)textPtr, text.c_str(), text.size() + 1)) return false; // Copy string
    return ReplaceStringWithPointer(offset, insAddr, strAddr, (u32)textPtr, reg);
}

void PatchMenuCustomLayoutDefault() {
    ReplaceConstString(BASE_OFF, 0x16edd8, 0x16f170, 9, "      Play"); // menu.play
    ReplaceConstString(BASE_OFF, 0x16ef24, 0x16f1a0, 0, " "); // menu.multiplayer
    ReplaceConstString(BASE_OFF, 0x16f078, 0x16f1b8, 0, " "); // menu.options
    ReplaceConstString(BASE_OFF, 0x16f228, 0x16f600, 0, " "); // menu.skins
    ReplaceConstString(BASE_OFF, 0x16f37c, 0x16f610, 0, CTRPF::Utils::Format("  LunaCore %d.%d.%d", PLG_VER_MAJ, PLG_VER_MIN, PLG_VER_PAT)); // menu.achievements
    ReplaceConstString(BASE_OFF, 0x16f4c8, 0x16f628, 0, " "); // menu.manual
    ReplaceConstString(BASE_OFF, 0x16f650, 0x16f884, 0, " "); // menu.store

    CTRPF::Process::Write8(0x16ef0c+BASE_OFF, 0x1d); // Menu buttons width (All but Play)
    CTRPF::Process::Write8(0x16edc4+BASE_OFF, 116); // Menu buttons width (Only Play)

    CTRPF::Process::Write8(0x16edd4+BASE_OFF, 66); // Menu buttons height (Play)
    CTRPF::Process::Write8(0x16ef18+BASE_OFF, 0x1c); // Menu buttons height (Multiplayer)
    CTRPF::Process::Write8(0x16f070+BASE_OFF, 0x1c); // Menu buttons height (Options)
    CTRPF::Process::Write8(0x16f220+BASE_OFF, 0x1c); // Menu buttons height (Skins)
    CTRPF::Process::Write8(0x16f364+BASE_OFF, 0x1c); // Menu buttons height (Achievements, Manual, Store)

    CTRPF::Process::Write8(0x16edc0+BASE_OFF, 120); // Menu buttons pos x (Play)
    CTRPF::Process::Write8(0x16ef1c+BASE_OFF, 255); // Menu buttons pos x (Multiplayer)
    CTRPF::Process::Write8(0x16f06c+BASE_OFF, 255); // Menu buttons pos x (Options)
    CTRPF::Process::Write8(0x16f214+BASE_OFF, 15); // Menu buttons pos x (Skins)
    CTRPF::Process::Write8(0x16f370+BASE_OFF, 15 + 0x1c + 10); // Menu buttons pos x (Achievements)
    CTRPF::Process::Write8(0x16f4b4+BASE_OFF, 255 - 0x1c - 10); // Menu buttons pos x (Manual)
    CTRPF::Process::Write8(0x16f640+BASE_OFF, 255); // Menu buttons pos x (Store)

    CTRPF::Process::Write8(0x16eeb0+BASE_OFF, 0); // Menu buttons submenuID (Play)
    CTRPF::Process::Write8(0x16ef2c+BASE_OFF, 5); // Menu buttons submenuID (Multiplayer)
    CTRPF::Process::Write8(0x16f060+BASE_OFF, 1); // Menu buttons submenuID (Options)
    CTRPF::Process::Write8(0x16f374+BASE_OFF, 3); // Menu buttons submenuID (Achievements)
    CTRPF::Process::Write8(0x16f4c0+BASE_OFF, 4); // Menu buttons submenuID (Manual)
    CTRPF::Process::Write8(0x16f5f4+BASE_OFF, 6); // Menu buttons submenuID (Store)

    CTRPF::Process::Write8(0x91a168, 75); // Menu buttons pox y (Play)
    CTRPF::Process::Write8(0x91a16c, 75); // Menu buttons pox y (Multiplayer)
    CTRPF::Process::Write8(0x91a170, 75 + 0x1c + 10); // Menu buttons pox y (Options)
    CTRPF::Process::Write8(0x91a174, 206); // Menu buttons pox y (Skins)
    CTRPF::Process::Write8(0x91a178, 206); // Menu buttons pox y (Achievements)
    CTRPF::Process::Write8(0x91a17c, 206); // Menu buttons pox y (Manual)
    CTRPF::Process::Write8(0x91a180, 206); // Menu buttons pox y (Store)

    CTRPF::Process::Write8(0x16edbc+BASE_OFF, 1); // Menu buttons icon scale (Play)
    CTRPF::Process::Write32(0x16ee04+BASE_OFF, 0xe3a02000);
}