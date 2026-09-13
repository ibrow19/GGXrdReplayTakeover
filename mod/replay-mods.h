#pragma once

#include <common.h>

// When a player's input is disabled this can be cast to the ID of the disabled player
enum class DisableInputMode : DWORD
{
    DisableP1 = 0,
    DisableP2 = 1,
    None = 2,
};

namespace ReplayDetourSettings
{
    // Detours will be used to set this to true when a frame step is initiated
    // by the regular (non-modded) replay controls. This communicates to the
    // replay controller that game state needs to be recorded next frame even
    // though the game is otherwise paused. The replay controller is
    // responsible for clearing this flag once the recording is handled.
    inline bool bReplayFrameStep = false;

    // The way we pause the engine does not prevent simple actors like
    // projectiles/vfx from updating. So we detour their tick and manually
    // prevent them from updating while we are paused. However, when navigating
    // the replay we may need to update their state while the game is otherwise
    // paused, this is handled by enabling this flag.
    inline bool bOverrideSimpleActorPause = false;

    // Controls detouring of input handling functions to optionally disable
    // whether one player's inputs get registered. Used at round start with
    // replay takeover so that only taken over player's inputs can be buffered.
    inline DisableInputMode disableInput = DisableInputMode::None;
}

// Attach/Detach replay related function detours and handle inialisation of 
// associated settings. Also handles adjustments to instructions required for
// replay behaviour modifications.
void AddReplayMods();
void RemoveReplayMods();

void DisableInputDisplay();
void EnableInputDisplay();
