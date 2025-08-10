#include "PluginInit.hpp"

#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <malloc.h>

#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"
#include "Core/TCPConnection.hpp"
#include "Core/Config.hpp"
#include "Core/Debug.hpp"

using namespace CTRPluginFramework;

extern int scriptsLoadedCount;
extern std::unordered_map<std::string, std::string> config;
extern PluginMenu *gmenu;
u32 *socBuffer = NULL;

void initSockets()
{
	socBuffer = (u32*)memalign(0x1000, 0x100000);
	if (!socBuffer)
		return;

	Result res = socInit(socBuffer, 0x100000);
	if (R_FAILED(res))
		return;
}

void exitSockets()
{
	socExit();
	free(socBuffer);
}

bool CancelOperationCallback() {
    Controller::Update();
    if (Controller::IsKeyDown(Key::B))
        return false;
    return true;
}

void InitMenu(PluginMenu &menu)
{
    // Create your entries here, or elsewhere
    // You can create your entries whenever/wherever you feel like it
    
    // Example entry
    /*menu += new MenuEntry("Test", nullptr, [](MenuEntry *entry)
    {
        std::string body("What's the answer ?\n");

        body += std::to_string(42);

        MessageBox("UA", body)();
    });*/
    auto optionsFolder = new MenuFolder("Options");
    auto devFolder = new MenuFolder("Developer");
    optionsFolder->Append(new MenuEntry("Toggle Script Loader", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (config["enable_scripts"] == "true") {
            if (MessageBox("Do you want to DISABLE script loader?", DialogType::DialogYesNo)()) {
                config["enable_scripts"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to ENABLE script loader?", DialogType::DialogYesNo)()) {
                config["enable_scripts"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Menu Layout", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (config["custom_game_menu_layout"] == "true") {
            if (MessageBox("Do you want to DISABLE custom menu layout?", DialogType::DialogYesNo)()) {
                config["custom_game_menu_layout"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to ENABLE custom menu layout?", DialogType::DialogYesNo)()) {
                config["custom_game_menu_layout"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Block ZL and ZR keys", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (config["disable_zl_and_zr"] == "true") {
            if (MessageBox("Do you want to ENABLE ZL and ZR keys?", DialogType::DialogYesNo)()) {
                config["disable_zl_and_zr"] = "false";
                Process::Write32(0x919530, KEY_ZL); // Pos keycode for ZL
                Process::Write32(0x919534, KEY_ZR); // Pos keycode for ZR
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to DISABLE ZL and ZR keys (only scripts will be able to use them)?", DialogType::DialogYesNo)()) {
                config["disable_zl_and_zr"] = "true";
                Process::Write32(0x919530, 0); // Pos keycode for ZL
                Process::Write32(0x919534, 0); // Pos keycode for ZR
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Block DPADLEFT and DPADRIGHT keys", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (config["disable_zl_and_zr"] == "true") {
            if (MessageBox("Do you want to ENABLE DPADLEFT and DPADRIGHT keys?", DialogType::DialogYesNo)()) {
                config["disable_dleft_and_dright"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to DISABLE DPADLEFT and DPADRIGHT keys (only scripts will be able to use them)?", DialogType::DialogYesNo)()) {
                config["disable_dleft_and_dright"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    devFolder->Append(new MenuEntry("Load script from network", nullptr, [](MenuEntry *entry) {
        initSockets();
        Core::Network::TCPServer tcp(5432);
        std::string host = tcp.getHostName();
        const Screen& topScreen = OSD::GetTopScreen();
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        OSD::SwapBuffers();
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        if (!tcp.waitConnection(CancelOperationCallback)) {
            exitSockets();
            if (!tcp.aborted)
                MessageBox("Connection error")();
            return;
        }
        size_t fnameSize = 0;
        if (!tcp.recv(&fnameSize, sizeof(size_t))) {
            exitSockets();
            MessageBox("Failed to get filename size")();
            return;
        }
        char *namebuf = new char[fnameSize];
        if (!namebuf) {
            exitSockets();
            MessageBox("Memory error")();
            return;
        }
        if (namebuf == NULL || !tcp.recv(namebuf, fnameSize)) {
            exitSockets();
            delete namebuf;
            MessageBox("Failed to get filename")();
            return;
        }

        size_t size = 0;
        if (!tcp.recv(&size, sizeof(size_t))) {
            exitSockets();
            delete namebuf;
            MessageBox("Failed to get file size")();
            return;
        }
        char *buffer = new char[size];
        if (!buffer) {
            exitSockets();
            delete namebuf;
            MessageBox("Memory error")();
            return;
        }
        if (!tcp.recv(buffer, size)) {
            exitSockets();
            delete namebuf;
            delete buffer;
            MessageBox("Failed to get file")();
            return;
        }

        if (Lua_Global_Mut.try_lock() && Core::LoadBuffer(buffer, size, ("net:/"+std::string(namebuf)).c_str())) {
            MessageBox("Script loaded")();
            if (MessageBox("Do you want to save this script to the sd card?", DialogType::DialogYesNo)()) {
                if (!Directory::IsExists(PLUGIN_FOLDER "/scripts/"))
                    Directory::Create(PLUGIN_FOLDER "/scripts/");
                File scriptOut;
                File::Open(scriptOut, PLUGIN_FOLDER "/scripts/"+std::string(namebuf), File::WRITE|File::CREATE);
                if (!scriptOut.IsOpen()) {
                    MessageBox("Failed to write to sd card")();
                } else {
                    scriptOut.Clear();
                    scriptOut.Write(buffer, size);
                    scriptOut.Flush();
                }
            }
        } else {
            MessageBox("Error executing the script")();
        }
        exitSockets();
        delete namebuf;
        delete buffer;
        Lua_Global_Mut.unlock();
    }));
    devFolder->Append(new MenuEntry("Clean Lua environment", nullptr, [](MenuEntry *entry) {
        if (!MessageBox("This will unload all loaded scripts. Continue?", DialogType::DialogYesNo)())
            return;
        if (!Lua_Global_Mut.try_lock())
            return
        Core::Debug::LogMessage("Reloading Lua environment", false);
        lua_close(Lua_global);
        Lua_global = luaL_newstate();
        Core::LoadLuaEnv();
        scriptsLoadedCount = 0;
        MessageBox("Lua environment reloaded")();
        Lua_Global_Mut.unlock();
    }));
    devFolder->Append(new MenuEntry("Reload scripts", nullptr, [](MenuEntry *entry) {
        if (!MessageBox("This will reload saved scripts, not including loaded by network (if not saved to sd card). Continue?", DialogType::DialogYesNo)())
            return;
        if (!Lua_Global_Mut.try_lock())
            return
        Core::Debug::LogMessage("Reloading Lua environment", false);
        lua_close(Lua_global);
        Lua_global = luaL_newstate();
        Core::LoadLuaEnv();
        scriptsLoadedCount = 0;
        Core::PreloadScripts();
        MessageBox("Lua environment reloaded")();
        Lua_Global_Mut.unlock();
    }));
    menu.Append(optionsFolder);
    menu.Append(devFolder);
}