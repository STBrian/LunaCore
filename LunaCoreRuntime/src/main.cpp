#include <3ds.h>
#include "csvc.h"
#include <CTRPluginFramework.hpp>
#include <FsLib/fslib.hpp>

#include <string>
#include <unordered_map>

#include <cstring>

#include "lua_common.h"
#include "Core/Debug.hpp"
#include "Core/Event.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Graphics/Graphics.hpp"
#include "Core/Utils/Utils.hpp"
#include "Core/Config.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Scheduler.hpp"

#include "Core/Hooks/GameHooks.hpp"
#include "Core/Hooks/MainMenuLayoutLoad.hpp"
#include "Core/Hooks/LoadingWorldScreenMessage.hpp"

#include "CoreInit.hpp"
#include "PluginInit.hpp"
#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"

#include "game/Minecraft.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;

CTRPF::PluginMenu *gmenu;

// Note Linux: using commit 14f1aff3

namespace CTRPluginFramework
{
    // This patch the NFC disabling the touchscreen when scanning an amiibo, which prevents ctrpf to be used
    static void    ToggleTouchscreenForceOn(void)
    {
        static u32 original = 0;
        static u32 *patchAddress = nullptr;

        if (patchAddress && original)
        {
            *patchAddress = original;
            return;
        }

        static const std::vector<u32> pattern =
        {
            0xE59F10C0, 0xE5840004, 0xE5841000, 0xE5DD0000,
            0xE5C40008, 0xE28DD03C, 0xE8BD80F0, 0xE5D51001,
            0xE1D400D4, 0xE3510003, 0x159F0034, 0x1A000003
        };

        Result  res;
        Handle  processHandle;
        s64     textTotalSize = 0;
        s64     startAddress = 0;
        u32 *   found;

        if (R_FAILED(svcOpenProcess(&processHandle, 16)))
            return;

        svcGetProcessInfo(&textTotalSize, processHandle, 0x10002);
        svcGetProcessInfo(&startAddress, processHandle, 0x10005);
        if(R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, processHandle, (u32)startAddress, textTotalSize)))
            goto exit;

        found = (u32 *)Utils::Search<u32>(0x14000000, (u32)textTotalSize, pattern);

        if (found != nullptr)
        {
            original = found[13];
            patchAddress = (u32 *)PA_FROM_VA((found + 13));
            found[13] = 0xE1A00000;
        }

        svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, textTotalSize);
        exit:
        svcCloseHandle(processHandle);
    }

    // This function is called before main and before the game starts
    // Useful to do code edits safely
    void    PatchProcess(FwkSettings &settings)
    {
        if (!Core::Utils::checkTitle())
            return;
        settings.UseGameHidMemory = true;
        ToggleTouchscreenForceOn();
        System::OnAbort = CrashHandler::OnAbort;
        Process::exceptionCallback = CrashHandler::ExceptionCallback;
        Process::ThrowOldExceptionOnCallbackException = true;
        CrashHandler::ReserveMemory();

        if (!fslib::initialize()) abort();

        if (!fslib::directory_exists(path_from_string(PLUGIN_FOLDER)))
            fslib::create_directory(path_from_string(PLUGIN_FOLDER));
        if (!Debug::OpenLogFile(LOG_FILE))
            OSD::Notify(Utils::Format("Failed to open log file '%s'", LOG_FILE));
        Debug::LogInfof("LunaCore version: %d.%d.%d", PLG_VER_MAJ, PLG_VER_MIN, PLG_VER_PAT);
        Debug::LogInfof("Loading config file '%s'", CONFIG_FILE);
        G_config = Config::LoadConfig(CONFIG_FILE);

        bool disableZLandZR = Config::GetBoolValue(G_config, "disable_zl_and_zr", false);
        bool disableDLandDR = Config::GetBoolValue(G_config, "disable_dleft_and_dright", false);

        if (Core::Utils::checkCompatibility() || System::IsCitra()) {
            Minecraft::PatchProcess();
            SetMainMenuLayoutLoadCallback();
            SetLoadingWorldScreenMessageCallback();
            SetLeaveLevelPromptCallback();
            if (disableZLandZR) {
                Process::Write32(0x919530, 0); // Pos keycode for ZL
                Process::Write32(0x919534, 0); // Pos keycode for ZR
            }
            if (disableDLandDR) {
                Process::Write32(0x919530-(4*8), 0); // Pos keycode for DPADRIGHT
                Process::Write32(0x919530-(4*7), 0); // Pos keycode for DPADLEFT
            }
            hookSomeFunctions();
            patchEnabled = true;
        }
    }

    // This function is called when the process exits
    // Useful to save settings, undo patchs or clean up things
    void    OnProcessExit(void)
    {
        ToggleTouchscreenForceOn();
        Core::CrashHandler::plg_state = Core::CrashHandler::PLUGIN_EXIT;
        lua_close(Lua_global);

        // Cleanup
        Core::Debug::LogInfo("Exiting LunaCore");
        Core::Debug::CloseLogFile();
        fslib::close_device(u"extdata");
        fslib::exit();
    }

    int main()
    {
        CrashHandler::plg_state = CrashHandler::PLUGIN_MAIN;
        if (!Core::Utils::checkTitle())
            return 0;

        if (!fslib::directory_exists(path_from_string(PLUGIN_FOLDER"/layouts")))
            fslib::create_directory(path_from_string(PLUGIN_FOLDER"/layouts"));

        bool loadMenuLayout = Config::GetBoolValue(G_config, "custom_game_menu_layout", true);
        if (loadMenuLayout && patchEnabled) {
            if ((fslib::file_exists(path_from_string(PLUGIN_FOLDER"/layouts/menu_layout.json")) 
                && LoadGameMenuLayout(PLUGIN_FOLDER"/layouts/menu_layout.json")) || 
                (fslib::file_exists(path_from_string(PLUGIN_FOLDER"/LunaCore/layouts/menu_layout.json")) 
                && LoadGameMenuLayout(PLUGIN_FOLDER"/LunaCore/layouts/menu_layout.json"))
            )
                PatchGameMenuLayoutFunction();
        }

        CrashHandler::core_state = CrashHandler::CORE_LOADING_RUNTIME;
        Core::InitCore();

        // Update configs
        if (!Config::SaveConfig(CONFIG_FILE, G_config))
            Debug::LogInfo("Failed to save configs");

        gmenu = new PluginMenu("LunaCore", PLG_VER_MAJ, PLG_VER_MIN, PLG_VER_PAT,
            "Allows to execute Lua scripts and other features", 2);
        std::string& title = gmenu->Title();
        title.assign("LunaCore Plugin Menu");

        // Synnchronize the menu with frame event
        gmenu->SynchronizeWithFrame(true);
        gmenu->ShowWelcomeMessage(false);

        gmenu->Callback(Core::EventHandlerCallback);
        gmenu->Callback(Core::Scheduler::Update);

        // Init our menu entries & folders
        InitMenu(*gmenu);

        // Launch menu and mainloop
        Debug::LogInfo("Starting plugin mainloop");
        CrashHandler::plg_state = CrashHandler::PLUGIN_MAINLOOP;
        GameState.CoreLoaded.store(true);
        gmenu->Run();
        
        delete gmenu;

        // Exit plugin
        return 0;
    }
}