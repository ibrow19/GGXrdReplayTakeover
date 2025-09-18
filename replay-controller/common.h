#pragma once

#include <windows.h>
#include <Psapi.h>
#include <functional>

inline const char* AppName = "Replay Takeover";
inline const char* GameName = "GuiltyGearXrd.exe";

char* GetModuleOffset(const char* name);
DWORD GetEngineOffset(const char* moduleOffset);
void ForEachEntity(const std::function<void (DWORD)>& func);
void MakeRegionWritable(DWORD base, DWORD size);
