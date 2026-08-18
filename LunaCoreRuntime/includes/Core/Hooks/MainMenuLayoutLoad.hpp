#pragma once

#include "CoreGlobals.hpp"

bool LoadGameMenuLayout(const std::string &filepath);

namespace Core::Hooks {
    void InstallMainMenuHooks();
}