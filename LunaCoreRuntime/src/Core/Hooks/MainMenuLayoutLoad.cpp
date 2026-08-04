#include "Core/Hooks/MainMenuLayoutLoad.hpp"

#include <vector>

#include <CTRPluginFramework.hpp>

#define JSON_NO_IO
#define JSON_NOEXCEPTION
#include "json.hpp"

#include "string_hash.hpp"

#include "Core/CrashHandler.hpp"
#include "CoreGlobals.hpp"
#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"
#include "Core/Hooks/GameHooks.hpp"
#include "Core/Screens/SettingsScreen.hpp"

#include <MC3DSPluginFramework.hpp>
#include <Minecraft/Common/Client/Gui/Screens/ScreenChooser.hpp>

#define GSTD_EXCLUDE_NAMESPACE
#include "game/gmalloc.h"

using json = nlohmann::ordered_json;
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

        LunaCore_Custom1 = 12,
    } MenuButtonID;
}

struct MenuBtnData {
    int id = 0;
    int x = 0, y = 0, width = 0, height = 0;
    struct subtext {
        unsigned int u = 0, v = 0, w = 0, h = 0;

        subtext() {}

        subtext(const MC3DSPluginFramework::IntRectangle& rect) :
        u(rect.mX), v(rect.mY), w(rect.mWidth), h(rect.mHeight) {}

        operator MC3DSPluginFramework::IntRectangle() {
            return MC3DSPluginFramework::IntRectangle(u, v, w, h);
        }
    } iconSubtex;
    std::string text;
    bool bigIcon = false;
};

struct MenuChrtData {
    float x = 0.0f, y = 0.0f;
    int width = 0, height = 0;
};

class ScreenProxy : public MC3DSPluginFramework::Screen {
    using GuiButton = MC3DSPluginFramework::GuiButton;
    template <typename T>
    using BoxedPtr = MC3DSPluginFramework::BoxedPtr::Shared<T>;
    template <typename T>
    using vector = MC3DSPluginFramework::gstd::vector<T>;

    public:
    void addButton(BoxedPtr<GuiButton>&& obj) {
        this->mButtons.push_back(std::move(obj));
    }

    void setupTabs() {
        this->mTabButtons = this->mButtons;
    }
};

class IconButton2 : public MC3DSPluginFramework::IconButton {
    using IntRectangle = MC3DSPluginFramework::IntRectangle;
    using ResourceLocation = MC3DSPluginFramework::ResourceLocation;

    using MinecraftGame = MC3DSPluginFramework::MinecraftGame;

    public:
    IconButton2(MinecraftGame *minecraftGame, int id, int x, int y, int w, int h, const char *localizeKey, bool bigIcon) :
        MC3DSPluginFramework::IconButton(minecraftGame, id, x, y, w, h, localizeKey, bigIcon) {}

    void setTextures(const ResourceLocation& location, const IntRectangle& iconTex, const IntRectangle& bgSubtexUV, const IntRectangle& hoveredSubtexUV) {
        this->setTexture(location, iconTex.mX, iconTex.mY, iconTex.mWidth, iconTex.mHeight, 0, bgSubtexUV, hoveredSubtexUV, 2, 2, 0);
    }
};

/* ------------------------------------------------------------------------------------- */

enum class ButtonSprite {
    PLAY,
    MULTIPLAYER,
    OPTIONS,
    SKINS,
    ACHIEVEMENTS,
    MANUAL,
    STORE,

    BG,
    BG_HOVERED,

    EMPTY
};

static const MC3DSPluginFramework::IntRectangle MenuSprites[] = {
    {224, 128, 16, 16},
    {224, 128, 16, 16},
    {224, 144, 16, 16},
    {224, 176, 16, 16},
    {224, 160, 16, 16},
    {240, 128, 16, 16},
    {240, 144, 16, 16},

    {64, 144, 8, 8},
    {56, 144, 8, 8},

    {0, 0, 8, 8}
};

static inline const MC3DSPluginFramework::IntRectangle& GetMenuSprite(ButtonSprite v) {
    return MenuSprites[(int)v];
}

/* ------------------------------------------------------------------------------------- */

static bool MainMenuLayoutLoaded = false;
static std::vector<MenuBtnData> MenuLayoutBtns;
static MenuChrtData MenuLayoutChrt;

/* ------------------------------------------------------------------------------------- */

static void CreateMenuButtons(int *ptr) {
    using namespace MC3DSPluginFramework;
    namespace MC3DSPF = MC3DSPluginFramework;

    const ResourceLocation &location = *(ResourceLocation *)0xABFD74;

    // --- Define all buttons ---
    for (auto &btnData : MenuLayoutBtns) {
        BoxedPtr::Shared<IconButton2> newButton(gstd::make_unique<IconButton2>(
            (MinecraftGame*)ptr[1], btnData.id, btnData.x, btnData.y, btnData.width, btnData.height, btnData.text.c_str(), btnData.bigIcon));
        newButton->setTextures(location, btnData.iconSubtex, GetMenuSprite(ButtonSprite::BG), GetMenuSprite(ButtonSprite::BG_HOVERED));
        reinterpret_cast<ScreenProxy*>(ptr)->addButton(newButton);
    }

    reinterpret_cast<ScreenProxy*>(ptr)->setupTabs();
}

static void CreateMainMenuCustomLayout(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;

    struct GameCharacterView {
        char data[0x90];
    };

    void (*InitMenuCharacter)(GameCharacterView* chrPtr,int*,float,float,int,int,int,int) = (decltype(InitMenuCharacter))(0x1ec930);
    void (*CreateUpdatePopUp)(int*,int,int,int) = (decltype(CreateUpdatePopUp))(0x26eb1c);

    *(char *)0xA35877 = 1;
    CreateMenuButtons(ptr);

    // --- Character container ---
    GameCharacterView *dataPtr = (GameCharacterView*)gstd_malloc(sizeof(GameCharacterView));
    if (dataPtr != NULL) {
        InitMenuCharacter(dataPtr, (int*)ptr[1], MenuLayoutChrt.x, MenuLayoutChrt.y, 0x32, 0x50, 2, 1);
    }

    ptr[0x24] = (int)dataPtr;
    CreateUpdatePopUp(ptr, ptr[1], 0xf0, 0x90); // Pop-up message

    return;
}

static void MainMenuLayoutLoadCallback(int *ptr) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;

    bool custom1Loaded = false;
    if (G_config.getBool("custom_game_menu_layout", true) && MainMenuLayoutLoaded) {
        CreateMainMenuCustomLayout(ptr);

        for (auto &btn : MenuLayoutBtns) {
            if (btn.id == MenuButtonID::LunaCore_Custom1) {
                custom1Loaded = true;
                break;
            }
        }
    } else {
        // MainMenuScreen::setupComponents(ptr);
        reinterpret_cast<void(*)(int*)>(0x26eda4)(ptr);
    }

    if (!custom1Loaded) {
        using namespace MC3DSPluginFramework;
        ScreenProxy* screen = reinterpret_cast<ScreenProxy*>(ptr);
        const ResourceLocation &location = *(ResourceLocation *)0xABFD74;

        // Add custom1 button
        BoxedPtr::Shared<IconButton2> newButton(gstd::make_unique<IconButton2>(
            (MinecraftGame*)ptr[1], MenuButtonID::LunaCore_Custom1, 10, 206, 28, 28, "", false));

        newButton->setTextures(location, GetMenuSprite(ButtonSprite::OPTIONS), GetMenuSprite(ButtonSprite::BG), GetMenuSprite(ButtonSprite::BG_HOVERED));
        screen->addButton(newButton);
        screen->setupTabs();
    }

    GameState.MainMenuLoaded.store(true);
    return;
}

static void MainMenuCustomButtonsHook(CoreHookContext* ctx) {
    using namespace MC3DSPluginFramework;

    u32 id = *(u32*)(ctx->r[5] + 0x78);
    if (id == MenuButtonID::LunaCore_Custom1) {
        ScreenChooser& screenChosser_ins = Facade::getMinecraftGame()->getScreenChooser();
        MinecraftGame* mg = Facade::getMinecraftGame();
        ClientInstance* ci = Facade::getClient();
        BoxedPtr::Shared<Core::LunaCoreSettingsScreen> screen_ins(gstd::make_unique<Core::LunaCoreSettingsScreen>(mg, ci));
        screenChosser_ins._pushScreen(screen_ins);
        // CTRPF::PluginMenu::ForceOpen();
    }
}

void Core::Hooks::InstallMainMenuHooks() {
    hookFunction(0x0026e878, (u32)MainMenuCustomButtonsHook); // Hook MainMenuScreen::buttonPressed
    CTRPF::Process::Write32(0x9ab4a4, (u32)MainMenuLayoutLoadCallback); // Patch vtable reference of MainMenuScreen::setupComponents
}

static void LoadButtonData(json &j, MenuBtnData &btnData) {
    btnData.x = j.value("x", 0);
    btnData.y = j.value("y", 0);
    btnData.width = j.value("width", 0);
    btnData.height = j.value("height", 0);
    btnData.text = j.value("text", "");
    btnData.bigIcon = j.value("bigIcon", false);

    bool hasIcon = false;
    if (j.contains("icon")) {
        if (j["icon"].is_array() && j["icon"].size() == 4) {
            json &icon = j["icon"];
            if (icon[0].is_number())
                btnData.iconSubtex.u = icon[0];
            if (icon[1].is_number())
                btnData.iconSubtex.v = icon[1];
            if (icon[2].is_number())
                btnData.iconSubtex.w = icon[2];
            if (icon[3].is_number())
                btnData.iconSubtex.h = icon[3];
            hasIcon = true;
        } else if (j["icon"].is_string()) {
            hasIcon = true;
            u32 textureId = hash(std::string(j["icon"]).c_str());
            switch (textureId)
            {
            case hash("play"): case hash("multiplayer"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::PLAY);
                break;

            case hash("options"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::OPTIONS);
                break;

            case hash("skins"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::SKINS);
                break;

            case hash("achievements"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::ACHIEVEMENTS);
                break;

            case hash("manual"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::MANUAL);
                break;

            case hash("store"):
                btnData.iconSubtex = GetMenuSprite(ButtonSprite::STORE);
                break;
            
            default:
                hasIcon = false;
                break;
            }
        }
    }

    if (!hasIcon)
        btnData.iconSubtex = GetMenuSprite(ButtonSprite::EMPTY);
    if (btnData.text.find("%d.%d.%d") != std::string::npos) 
        btnData.text = CTRPF::Utils::Format(btnData.text.c_str(), LUNACORE_VER_MAJOR, LUNACORE_VER_MINOR, LUNACORE_VER_PATCH);
}

bool LoadGameMenuLayout(const std::string& filepath) {
    Core::Debug::LogInfo("Loading menu layout: '" + filepath + "'");
    std::string fileCont = Core::Utils::LoadFile(filepath);
    if (!fileCont.empty()) {
        json j = json::parse(std::string(fileCont), nullptr, false);
        if (!j.is_discarded()) {
            if (j.contains("buttons") && j["buttons"].is_object()) {
                MenuLayoutBtns.clear();
                json& btnsData = j["buttons"];
                for (auto& [key, value] : btnsData.items()) {
                    if (value.is_object()) {
                        u32 key_hash = hash(key.c_str());
                        bool valid = true;
                        MenuBtnData btnData{};
                        switch (key_hash) 
                        {
                        case hash("play"): 
                            btnData.id = MenuButtonID::Play;
                            break;

                        case hash("multiplayer"):
                            btnData.id = MenuButtonID::Multiplayer;
                            break;

                        case hash("options"):
                            btnData.id = MenuButtonID::Options;
                            break;

                        case hash("skins"):
                            btnData.id = MenuButtonID::Skins;
                            break;

                        case hash("achievements"):
                            btnData.id = MenuButtonID::Achievements;
                            break;

                        case hash("manual"):
                            btnData.id = MenuButtonID::Manual;
                            break;

                        case hash("store"):
                            btnData.id = MenuButtonID::Store;
                            break;

                        case hash("custom1"):
                            btnData.id = MenuButtonID::LunaCore_Custom1;
                            break;

                        default:
                            valid = false;
                            break;
                        }
                        if (valid) {
                            LoadButtonData(value, btnData);
                            MenuLayoutBtns.push_back(btnData);
                        }
                    }
                }

                if (j.contains("character") && j["character"].is_object()) {
                    json& chrtData = j["character"];
                    if (chrtData.contains("x") && chrtData["x"].is_number())
                        MenuLayoutChrt.x = chrtData["x"];
                    if (chrtData.contains("y") && chrtData["y"].is_number())
                        MenuLayoutChrt.y = chrtData["y"];
                    MainMenuLayoutLoaded = true;
                    return true;
                }
            }
        }
    }
    Core::Debug::LogWarn("Failed to load main menu layout");
    return false;
}