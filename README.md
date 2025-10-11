# Guilty Gear Xrd Replay Takeover and Save States
This mod is still somewhat experimental, expect some bugs or crashes.
## Enabling the mod
- Grab the latest build from releases and unzip it.
- Open Guilty Gear Xrd.
- Run GGXrdReplayTakeoverInjector.exe

At the moment you're going to have to do this again each time you launch the game and want to use the mod but I'll add some steps to enable it automatically once I'm sure the mod is more stable.

## Using Replay Takeover
Enter a replay, a Replay Takeover window should pop up with info about the current replay takeover state.
<img width="367" height="109" alt="image" src="https://github.com/user-attachments/assets/a25fa7dc-605d-4a76-8706-46010fe3b513" />
### Modes
- DISABLED - The mod is disabled and the normal replay controls for pausing, chaning camera etc can be used.
- STANDBY - The mod is overriding the regular replay controls to enable rewinding and takeover but takeover is not currently active.
- TAKEOVER - Replay takeover is active controlling one of the players.

### Controls
All controls are based on your current bindings in the "Button Settings (Battle)" menu except for the reset training mode button which cannot be rebound as far as I'm aware and will usually be the equivalent of the select button on your controller.
Note that all controls will not be usable until Reset is pressed to override the default replay behaviour.

- Reset - Toggle between DISABLED and STANDBY
- P - Pause/unpause
- K - Toggle which player will be used for takeover.
- Left/Right - Go back or forward in the replay. Can be held.
- Up/down - Fast forward/rewind 3 frames at a time. Can be held.
- S/H - Step back or forward one frame at a time. Cannot be held so allows for more precise navigation than directions.
- Play - Initiate replay takeover on the current frame. If already in takeover then it will restart the takeover from the original frame.
- Record - Cancel the current takeover and go back to standby mode on the frame takeover began.

### Rewinding Limits
The mod builds a buffer of the past couple seconds as the replay progresses to enable rewinding. However, as each saved frame is ~2.24mb, saving too many frames quickly uses a massive amount of memory.
Currently the mod's fixed maximum for the rewind buffer is 500 frames (8 seconds), older frames will be lost and you'll need to restart the replay to get back to them. 
500 frames requires ~1.1GB to store, in the future it probably makes sense to make the buffer size configurable so you can use more/less depending on your needs.

### Takeover Countdown
The takeover countdown is a short delay before takeover takes effect after pressing play. The number of frames can be configured using the slider in the replay takeover window.

## Using Save States
Enter training mode, a Save State window should pop up with info about the current save state data.

<img width="225" height="104" alt="image" src="https://github.com/user-attachments/assets/30be80ec-e33d-4b12-8919-706b2a92d75a" />

### Controls
Save states uses contextual controls based on whether the "Special Move" button held. You probably won't have this button bound already since it's normally only used with stylish mode.

- Special Move + Record - Save State
- Special Move + Play - Load State

## Known Issues
- Many simple actors/visual effects display incorrectly when loaded from a save state as they always begin from their first frame of animation. Furthermore, these visual effects are not paused in replay takeover standby mode/rewinding.
- Some visual effects are not stored in the save states so will be absent from any loaded state. Furthermore their absence can cause other visual effects to appear in the wrong place. For example, Answer's scrolls have a green glow that is not part of the save state, the glow will be missing from states loaded that include scrolls.
- The camera angle for some cinematic supers is not correctly stored in save states. Loading a save state or initating replay takeover mid-way through a cinematic super can result in an incorrect camera angle until you exit the replay/training.
