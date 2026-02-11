#include <memory-wrapper.h>


class SoundData : public MemoryWrapper
{
public:
    SoundData(DWORD inPtr);
    float& GetBgmVolume();
    float& GetMenuEffectVolume();
    float& GetBattleEffectVolume();
    float& GetBattleVoiceVolume();
    float& GetBattleSuperVoiceVolume();
    float& GetBattleSystemVoiceVolume();
};
