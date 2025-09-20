#pragma once

#include <windows.h>
#include <functional>

inline const char* AppName = "Replay Takeover";

void ForEachEntity(const std::function<void (DWORD)>& func);
void MakeRegionWritable(DWORD base, DWORD size);
