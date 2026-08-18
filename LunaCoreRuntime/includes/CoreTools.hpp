#pragma once

#include <3ds/types.h>

#define SHAREDSTATE_MAGIC (('L' << 24) | ('C' << 16) | ('S' << 8) | ('S'))

enum LC_LaunchType : u32 {
    LC_LAUNCH_STANDALONE = 0,
    LC_LAUNCH_LAUNCHER,
    LC_LAUNCH_RESOURCEBUILDER
};

typedef struct LC_SharedState_s {
    u32 magic;
    LC_LaunchType launched_from;
} LC_SharedState;

#ifdef __LCRuntime__
// Only available in the runtime
namespace LunaCore {
void LaunchTool(const char* name);
}
#endif