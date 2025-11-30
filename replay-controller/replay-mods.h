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
void AttachReplayMods();
void DetachReplayMods();

void EnableSoundEffects();
void DisableSoundEffects();
