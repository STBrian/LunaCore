#include "PluginInit.hpp"

#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <malloc.h>
#include <memory>
#include <FsLib/fslib.hpp>

#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"
#include "Core/Config.hpp"
#include "Core/Debug.hpp"
#include "Core/Filesystem.hpp"

#include "Helpers/TCPConnection.hpp"

using namespace CTRPluginFramework;

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
    if (socBuffer != NULL) {
        socExit();
	    free(socBuffer);
        socBuffer = NULL;
    }
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
#include "Helpers/Threads.hpp"

void PCConnectionThreadFunction(Core::Network::TCPServer* tcp) {
    u32 cmdId = 0;
    while (cmdId != 0x5AB1E) {
        tcp->recv(&cmdId, 4);
        switch (cmdId) {
            case 0: // ping
                tcp->send_all(&cmdId, 4);
                break;
            case 1: { // write32 to offset
                u32 offset, value;
                tcp->recv(&offset, 4);
                tcp->recv(&value, 4);
                Process::Write32(offset, value);
                break;
            }
            case 2: { // read32 from offset
                u32 offset, value;
                tcp->recv(&offset, 4);
                Process::Read32(offset, value);
                tcp->send_all(&value, 4);
                break;
            }
            case 3: { // storfile
                u32 dstPathLen, fileSize;
                tcp->recv(&dstPathLen, 4);
                char* pathData = new (std::nothrow) char[dstPathLen];
                if (pathData) {
                    tcp->recv(pathData, dstPathLen);
                    pathData[dstPathLen-1] = '\0';
                    std::string pathName(pathData);
                    delete pathData;
                    tcp->recv(&fileSize, 4);
                    fslib::File dstFile;
                    dstFile.open(path_from_string(pathName), FS_OPEN_WRITE|FS_OPEN_CREATE);
                    char* buffer = new (std::nothrow) char[0x100];
                    if (buffer) {
                        u32 currentRecv = 0;
                        while (currentRecv < fileSize) {
                            u32 toRecv = fileSize - currentRecv > 0x100 ? 0x100 : fileSize - currentRecv;
                            tcp->recv(buffer, toRecv);
                            dstFile.write(buffer, toRecv);
                            currentRecv += toRecv;
                        }
                        dstFile.flush();
                        delete buffer;
                    }
                    dstFile.close();
                }
                break;
            }
        }
    }

    delete tcp;
}

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
                Core::Debug::LogWarn("Failed to save configs");
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
                Core::Debug::LogWarn("Failed to save configs");
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
                Core::Debug::LogWarn("Failed to save configs");
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
                Core::Debug::LogWarn("Failed to save configs");
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
    devFolder->Append(new MenuEntry("Init network", nullptr, [](MenuEntry *entry) {
        if (socBuffer == NULL) {
            initSockets();
            MessageBox("Network started")();
        } else
            MessageBox("Network already started")();
    }));
    devFolder->Append(new MenuEntry("Exit network", nullptr, [](MenuEntry *entry) {
        if (socBuffer != NULL) {
            exitSockets();
            MessageBox("Network exited")();
        } else
            MessageBox("Network is not started")();
    }));
    devFolder->Append(new MenuEntry("Load script from network", nullptr, [](MenuEntry *entry) {
        if (socBuffer == NULL) {
            MessageBox("Network is not started")();
            return;
        }
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
            if (!tcp.aborted)
                MessageBox("Connection error")();
            return;
        } 

        // Get the filename
        size_t fnameSize = 0;
        if (!tcp.recv(&fnameSize, sizeof(size_t))) {
            MessageBox("Failed to get filename size")();
            return;
        }
        std::unique_ptr<char[]> namebuf(new (std::nothrow) char[fnameSize]);
        if (!namebuf) {
            MessageBox("Memory error")();
            return;
        }
        if (!tcp.recv(namebuf.get(), fnameSize)) {
            MessageBox("Failed to get filename")();
            return;
        }

        // Get script file
        size_t size = 0;
        if (!tcp.recv(&size, sizeof(size_t))) {
            MessageBox("Failed to get file size")();
            return;
        }
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[size]);
        if (!buffer) {
            MessageBox("Memory error")();
            return;
        }
        if (!tcp.recv(buffer.get(), size)) {
            MessageBox("Failed to get file")();
            return;
        }

        if (!Lua_Global_Mut.try_lock())
            MessageBox("Error executing the script. Failed to lock lua")();
        else {
            if (Core::LoadBuffer(buffer.get(), size, ("net:/" + std::string(namebuf.get()).substr(0, fnameSize-1)).c_str())) {
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
            } else
                MessageBox("Error executing the script")();
            Lua_Global_Mut.unlock();
        }
    }));

    #ifdef DEBUG
    #pragma message "Debug enabled"
    devFolder->Append(new MenuEntry("Dump memory to network", nullptr, [](MenuEntry *entry) {
        if (socBuffer == NULL) {
            MessageBox("Network is not started")();
            return;
        }
        Core::Network::TCPServer tcp(5432);
        std::string host = tcp.getHostName();

        // Draw message
        const Screen& topScreen = OSD::GetTopScreen();
        topScreen.DrawRect(30, 20, 360, 200, Color::Black);
        topScreen.DrawSysfont("Connect to host: "+host+":5432", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        OSD::SwapBuffers();
        topScreen.DrawRect(30, 20, 360, 200, Color::Black);
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
                    MessageBox("Failed to send packets")();
                    return;
                }
                currentPosition += (remaining < packetSize ? remaining : packetSize);
                remaining = region.b - currentPosition;
                topScreen.DrawRect(30, 20, 360, 200, Color::Black);
                topScreen.DrawSysfont(Utils::Format("Progress: %.2f", (float)(regionSize - remaining)/regionSize * 100), 100, 100, Color::White);
                topScreen.DrawSysfont(Utils::Format("Dumping: %08X", region.a), 100, 110, Color::White);
                OSD::SwapBuffers();
            }
        }
    }));
    devFolder->Append(new MenuEntry("Connect to PC", nullptr, [](MenuEntry *entry) {
        if (socBuffer == NULL) {
            MessageBox("Network is not started")();
            return;
        }
        auto tcp = new Core::Network::TCPServer(5431);
        std::string host = tcp->getHostName();

        // Draw message
        const Screen& topScreen = OSD::GetTopScreen();
        topScreen.DrawRect(30, 20, 340, 200, Color::Black);
        topScreen.DrawSysfont("Connect to host: "+host+":5431", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);
        OSD::SwapBuffers();
        topScreen.DrawRect(30, 20, 340, 200, Color::Black);
        topScreen.DrawSysfont("Connect to host: "+host+":5431", 40, 185, Color::White);
        topScreen.DrawSysfont("Waiting connection... Press B to cancel", 40, 200, Color::White);

        // Wait until a connection
        if (!tcp->waitConnection(CancelOperationCallback)) {
            if (!tcp->aborted)
                MessageBox("Connection error")();
            delete tcp;
            return;
        }

        Core::Thread connThread(PCConnectionThreadFunction, tcp);
        if (!connThread.isStarted()) {
            MessageBox("Failed to start thread")();
            delete tcp;
        }
    }));
    #endif
    
    devFolder->Append(new MenuEntry("Clean Lua environment", nullptr, [](MenuEntry *entry) {
        if (!MessageBox("This will reload Lua env without any scripts or mods. Continue?", DialogType::DialogYesNo)())
            return;
        if (!Lua_Global_Mut.try_lock())
            return
        Core::Debug::Message("Reloading Lua environment");
        lua_close(Lua_global);
        Lua_global = luaL_newstate();
        Core::LoadLuaEnv();
        loadedScripts = 0;
        MessageBox("Lua environment cleaned")();
        Lua_Global_Mut.unlock();
    }));
    devFolder->Append(new MenuEntry("Reload Lua environment", nullptr, [](MenuEntry *entry) {
        if (!MessageBox("This will reload Lua env and load all scripts and mods. Continue?", DialogType::DialogYesNo)())
            return;
        if (!Lua_Global_Mut.try_lock())
            return
        Core::Debug::Message("Reloading Lua environment");
        lua_close(Lua_global);
        Lua_global = luaL_newstate();
        Core::LoadLuaEnv();
        Core::LoadMods();
        loadedScripts = 0;
        Core::PreloadScripts();
        MessageBox("Lua environment reloaded")();
        Lua_Global_Mut.unlock();
    }));
    menu.Append(MenuModsFolder);
    menu.Append(optionsFolder);
    menu.Append(devFolder);
}