#pragma once

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#define NUMBER_FILE_OP 9

namespace OnionFS {
    extern LightLock regLock;
    extern LightLock openLock;

    extern bool g_onionfs_enable_romfs_redir;
    extern bool g_onionfs_enable_extdata_redir;
    extern std::string g_onionfs_default_romfs_redir_path;
    extern std::string g_onionfs_default_extdata_redir_path;

    void InitFS(void);
    void ReloadConfig(void);
    void InitMenu(CTRPluginFramework::PluginMenu* menu);
}
