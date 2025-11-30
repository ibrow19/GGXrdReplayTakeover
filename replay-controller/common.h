#pragma once

#include <windows.h>

typedef void(*EntityFunc)(DWORD entity, DWORD index, void* extraData);

inline const char* AppName = "Replay Takeover";

void MakeRegionWritable(DWORD base, DWORD size);
void ForEachEntity(EntityFunc func, void* extraData = nullptr);
