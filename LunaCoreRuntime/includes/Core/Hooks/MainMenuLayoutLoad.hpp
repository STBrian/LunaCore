#pragma once

#include "CoreGlobals.hpp"

void PatchGameMenuLayoutFunction();

bool LoadGameMenuLayout(const std::string &filepath);

void SetMainMenuLayoutLoadCallback();