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

#include "GameAPI.hpp"

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
    MinecraftAPI::Resource<MinecraftAPI::GuiAtlas> icon;
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

/* ------------------------------------------------------------------------------------- */

static bool MainMenuLayoutLoaded = false;
static std::vector<MenuBtnData> MenuLayoutBtns;
static MenuChrtData MenuLayoutChrt;

/* ------------------------------------------------------------------------------------- */

static void CreateMenuButtons(int *ptr) {
    using namespace MinecraftAPI;

    const ResourceLocation &location = *(ResourceLocation *)0xABFD74;

    // --- Define all buttons ---
    for (auto &btnData : MenuLayoutBtns) {
        BoxedPtr::Shared<IconButton> newButton(gstd::make_unique<IconButton>(
            (MinecraftGame*)ptr[1], btnData.id, btnData.x, btnData.y, btnData.width, btnData.height, btnData.text.c_str(), btnData.bigIcon));
        newButton->setTextures(btnData.icon, GuiAtlas::BUTTON_BG, GuiAtlas::BUTTON_BG_HOVERED, 2, 2);
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
        using namespace MinecraftAPI;
        ScreenProxy* screen = reinterpret_cast<ScreenProxy*>(ptr);
        const ResourceLocation &location = *(ResourceLocation *)0xABFD74;

        // Add custom1 button
        BoxedPtr::Shared<IconButton> newButton(gstd::make_unique<IconButton>(
            (MinecraftGame*)ptr[1], MenuButtonID::LunaCore_Custom1, 10, 206, 28, 28, "", false));

        newButton->setTextures(GuiAtlas::OPTIONS_ICON, GuiAtlas::BUTTON_BG, GuiAtlas::BUTTON_BG_HOVERED, 2, 2);
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
                btnData.icon.region.mX = icon[0];
            if (icon[1].is_number())
                btnData.icon.region.mY = icon[1];
            if (icon[2].is_number())
                btnData.icon.region.mWidth = icon[2];
            if (icon[3].is_number())
                btnData.icon.region.mHeight = icon[3];
            hasIcon = true;
        } else if (j["icon"].is_string()) {
            hasIcon = true;
            u32 textureId = hash(std::string(j["icon"]).c_str());
            switch (textureId)
            {
            case hash("play"): case hash("multiplayer"):
                btnData.icon = MinecraftAPI::GuiAtlas::PLAY_ICON;
                break;

            case hash("options"):
                btnData.icon = MinecraftAPI::GuiAtlas::OPTIONS_ICON;
                break;

            case hash("skins"):
                btnData.icon = MinecraftAPI::GuiAtlas::SKINS_ICON;
                break;

            case hash("achievements"):
                btnData.icon = MinecraftAPI::GuiAtlas::ACHIEVEMENTS_ICON;
                break;

            case hash("manual"):
                btnData.icon = MinecraftAPI::GuiAtlas::MANUAL_ICON;
                break;

            case hash("store"):
                btnData.icon = MinecraftAPI::GuiAtlas::STORE_ICON;
                break;
            
            default:
                hasIcon = false;
                break;
            }
        }
    }

    if (!hasIcon)
        btnData.icon = MinecraftAPI::GuiAtlas::EMPTY;
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