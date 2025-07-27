#pragma once

#include <windows.h>
#include <Psapi.h>

inline const char* GAppName = "Replay Controller";
inline const char* GGameName = "GuiltyGearXrd.exe";

char* GetModuleOffset(const char* name);
