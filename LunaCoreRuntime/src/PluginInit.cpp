#include "PluginInit.hpp"

#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <malloc.h>
#include <memory>

#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"
#include "Core/TCPConnection.hpp"
#include "Core/Config.hpp"
#include "Core/Debug.hpp"

using namespace CTRPluginFramework;

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

bool DrawMonitors(const Screen &screen) {
    static int luaMemoryUsage = 0;
    if (screen.IsTop) {
        if (Lua_Global_Mut.try_lock()) {
            int memusgkb = lua_gc(Lua_global, LUA_GCCOUNT, 0);
            int memusgb = lua_gc(Lua_global, LUA_GCCOUNTB, 0);
            luaMemoryUsage = memusgkb * 1024 + memusgb;
            Lua_Global_Mut.unlock();
        }
        screen.Draw("Lua memory: "+std::to_string(luaMemoryUsage), 5, 5, Color::Black, Color(0, 0, 0, 0));
    }
    return false;
}

#ifdef DEBUG
struct _pair {
    u32 a, b;
};
#endif

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
        if (G_config["enable_scripts"] == "true") {
            if (MessageBox("Do you want to DISABLE script loader?", DialogType::DialogYesNo)()) {
                G_config["enable_scripts"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to ENABLE script loader?", DialogType::DialogYesNo)()) {
                G_config["enable_scripts"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, G_config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Menu Layout", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (G_config["custom_game_menu_layout"] == "true") {
            if (MessageBox("Do you want to DISABLE custom menu layout?", DialogType::DialogYesNo)()) {
                G_config["custom_game_menu_layout"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to ENABLE custom menu layout?", DialogType::DialogYesNo)()) {
                G_config["custom_game_menu_layout"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, G_config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Block ZL and ZR keys", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (G_config["disable_zl_and_zr"] == "true") {
            if (MessageBox("Do you want to ENABLE ZL and ZR keys?", DialogType::DialogYesNo)()) {
                G_config["disable_zl_and_zr"] = "false";
                Process::Write32(0x919530, KEY_ZL); // Pos keycode for ZL
                Process::Write32(0x919534, KEY_ZR); // Pos keycode for ZR
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to DISABLE ZL and ZR keys (only scripts will be able to use them)?", DialogType::DialogYesNo)()) {
                G_config["disable_zl_and_zr"] = "true";
                Process::Write32(0x919530, 0); // Pos keycode for ZL
                Process::Write32(0x919534, 0); // Pos keycode for ZR
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, G_config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Toggle Block DPADLEFT and DPADRIGHT keys", nullptr, [](MenuEntry *entry)
    {
        bool changed = false;
        if (G_config["disable_zl_and_zr"] == "true") {
            if (MessageBox("Do you want to ENABLE DPADLEFT and DPADRIGHT keys?", DialogType::DialogYesNo)()) {
                G_config["disable_dleft_and_dright"] = "false";
                changed = true;
            }
        } else {
            if (MessageBox("Do you want to DISABLE DPADLEFT and DPADRIGHT keys (only scripts will be able to use them)?", DialogType::DialogYesNo)()) {
                G_config["disable_dleft_and_dright"] = "true";
                changed = true;
            }
        }
        if (changed) {
            MessageBox("Restart the game to apply the changes")();
            if (!Core::Config::SaveConfig(CONFIG_FILE, G_config))
                Core::Debug::LogMessage("Failed to save configs", true);
        }
    }));
    optionsFolder->Append(new MenuEntry("Show Lua memory usage", nullptr, [](MenuEntry *entry)
    {
        static bool enabled = false;
        if (!enabled)
            OSD::Run(DrawMonitors);
        else
            OSD::Stop(DrawMonitors);
        enabled = !enabled;
    }));
    devFolder->Append(new MenuEntry("Load script from network", nullptr, [](MenuEntry *entry) {
        initSockets();
        Core::Network::TCPServer tcp(5432);
        std::string host = tcp.getHostName();

        // Draw tip message
        const Screen& topScreen = OSD::GetTopScreen();
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        OSD::SwapBuffers();
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);

        // Wait for a connection
        if (!tcp.waitConnection(CancelOperationCallback)) {
            exitSockets();
            if (!tcp.aborted)
                MessageBox("Connection error")();
            return;
        } 

        // Get the filename
        size_t fnameSize = 0;
        if (!tcp.recv(&fnameSize, sizeof(size_t))) {
            exitSockets();
            MessageBox("Failed to get filename size")();
            return;
        }
        std::unique_ptr<char[]> namebuf(new (std::nothrow) char[fnameSize]);
        if (!namebuf) {
            exitSockets();
            MessageBox("Memory error")();
            return;
        }
        if (!tcp.recv(namebuf.get(), fnameSize)) {
            exitSockets();
            MessageBox("Failed to get filename")();
            return;
        }

        // Get script file
        size_t size = 0;
        if (!tcp.recv(&size, sizeof(size_t))) {
            exitSockets();
            MessageBox("Failed to get file size")();
            return;
        }
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[size]);
        if (!buffer) {
            exitSockets();
            MessageBox("Memory error")();
            return;
        }
        if (!tcp.recv(buffer.get(), size)) {
            exitSockets();
            MessageBox("Failed to get file")();
            return;
        }

        if (Lua_Global_Mut.try_lock() && Core::LoadBuffer(buffer.get(), size, ("net:/" + std::string(namebuf.get()).substr(0, fnameSize-1)).c_str())) {
            MessageBox("Script loaded")();
            if (MessageBox("Do you want to save this script to the sd card?", DialogType::DialogYesNo)()) {
                if (!Directory::IsExists(PLUGIN_FOLDER "/scripts/"))
                    Directory::Create(PLUGIN_FOLDER "/scripts/");
                File scriptOut;
                File::Open(scriptOut, PLUGIN_FOLDER "/scripts/" + std::string(namebuf.get()).substr(0, fnameSize-1), File::WRITE|File::CREATE);
                if (!scriptOut.IsOpen()) {
                    MessageBox("Failed to write to sd card")();
                } else {
                    scriptOut.Clear();
                    scriptOut.Write(buffer.get(), size);
                    scriptOut.Flush();
                }
            }
            Lua_Global_Mut.unlock();
        } else {
            MessageBox("Error executing the script")();
        }
        exitSockets();
    }));

    #ifdef DEBUG
    #pragma message "'Dump memory to network' menu entry enabled"
    devFolder->Append(new MenuEntry("Dump memory to network", nullptr, [](MenuEntry *entry) {
        initSockets();
        Core::Network::TCPServer tcp(5432);
        std::string host = tcp.getHostName();

        // Draw message
        const Screen& topScreen = OSD::GetTopScreen();
        topScreen.DrawRect(35, 21, 380, 200, Color::Black);
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        OSD::SwapBuffers();
        topScreen.DrawRect(35, 21, 380, 200, Color::Black);
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);

        std::vector<struct _pair> regions = {
            {0x100000, 0x919000},
            {0x919000, 0xa29000},
            {0xa29000, 0xa3a000},
            {0xa3b000, 0xa3c000},
            {0xa3d000, 0xb40000},
            {0x8000000, 0x8002000},
            {0x8006000, 0x800c000},
            {0x8010000, 0x801e000},
            {0x8020000, 0x8800000},
            {0xe000000, 0xe004000},
            {0xe005000, 0xe009000},
            {0xe00a000, 0xe00c000},
            {0x30000000, 0x35f80000}
        };

        // Wait until a connection
        if (!tcp.waitConnection(CancelOperationCallback)) {
            exitSockets();
            if (!tcp.aborted)
                MessageBox("Connection error")();
            return;
        }

        u32 totalRegions = regions.size();
        tcp.send_all(&totalRegions, sizeof(u32));
        for (auto& region : regions) {
            u32 regionSize = region.b - region.a;
            u32 remaining = regionSize;
            u32 currentPosition = region.a;
            u32 packetSize = 0x1000;
            tcp.send_all(&regionSize, sizeof(u32));
            while (currentPosition < region.b) {
                if (!tcp.send_all((void*)currentPosition, remaining < packetSize ? remaining : packetSize)) {
                    exitSockets();
                    MessageBox("Failed to send packets")();
                    return;
                }
                currentPosition += (remaining < packetSize ? remaining : packetSize);
                remaining = region.b - currentPosition;
                topScreen.DrawRect(35, 21, 380, 200, Color::Black);
                topScreen.DrawSysfont(Utils::Format("Progress: %.2f", (float)(regionSize - remaining)/regionSize * 100), 100, 100, Color::White);
                topScreen.DrawSysfont(Utils::Format("Dumping: %08X", region.a), 100, 110, Color::White);
                OSD::SwapBuffers();
            }
        }
        
        exitSockets();
    }));
    #endif
    
    devFolder->Append(new MenuEntry("Clean Lua environment", nullptr, [](MenuEntry *entry) {
        if (!MessageBox("This will unload all loaded scripts. Continue?", DialogType::DialogYesNo)())
            return;
        if (!Lua_Global_Mut.try_lock())
            return
        Core::Debug::LogMessage("Reloading Lua environment", false);
        lua_close(Lua_global);
        Lua_global = luaL_newstate();
        Core::LoadLuaEnv();
        loadedScripts = 0;
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
        loadedScripts = 0;
        Core::PreloadScripts();
        MessageBox("Lua environment reloaded")();
        Lua_Global_Mut.unlock();
    }));
    menu.Append(optionsFolder);
    menu.Append(devFolder);
}