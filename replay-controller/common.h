#pragma once

#include <windows.h>
#include <Psapi.h>

inline const char* AppName = "Replay Controller";
inline const char* GameName = "GuiltyGearXrd.exe";

char* GetModuleOffset(const char* name);
