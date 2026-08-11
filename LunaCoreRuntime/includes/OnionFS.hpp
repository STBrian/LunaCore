#pragma

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#define NUMBER_FILE_OP 9

namespace OnionFS {
    extern LightLock regLock;
    extern LightLock openLock;

    void InitFS(void);

    void InitMenu(CTRPluginFramework::PluginMenu* menu);
}