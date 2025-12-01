#pragma once

namespace ReplayDetourSettings
{
    inline bool bReplayFrameStep = false;
    inline bool bOverrideSimpleActorPause = false;
    inline bool bOverrideHudText = false;
    inline bool bAddingFirstTextRow = false;
}

// Attach/Detach replay related function detours and handle inialisation of 
// associated settings. Also handles adjustments to instructions required for
// replay behaviour modifications.
void AddReplayMods();
void RemoveReplayMods();

void DisableSoundEffects();
void EnableSoundEffects();

void DisableInputDisplay();
void EnableInputDisplay();
