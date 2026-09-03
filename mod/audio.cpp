#include <audio.h>


SoundData::SoundData(DWORD inPtr)
: MemoryWrapper(inPtr)
{}

float& SoundData::GetBgmVolume()
{
    return *(float*)(mPtr + 0x90);
}

float& SoundData::GetMenuEffectVolume()
{
    return *(float*)(mPtr + 0x38);
}

float& SoundData::GetBattleEffectVolume()
{
    return *(float*)(mPtr + 0x338);
}

float& SoundData::GetBattleVoiceVolume()
{
    return *(float*)(mPtr + 0xf8);
}

float& SoundData::GetBattleSuperVoiceVolume()
{
    return *(float*)(mPtr + 0x128);
}

float& SoundData::GetBattleSystemVoiceVolume()
{
    return *(float*)(mPtr + 0x308);
}
