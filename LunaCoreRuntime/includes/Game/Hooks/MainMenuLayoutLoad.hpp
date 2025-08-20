#pragma once

#include "CoreGlobals.hpp"

void PatchGameMenuLayoutFunction();

bool LoadGameMenuLayout(const STRING_CLASS &filepath);

void PatchMenuCustomLayoutDefault();

void SetMainMenuLayoutLoadCallback();