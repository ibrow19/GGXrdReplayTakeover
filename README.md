# Guilty Gear Xrd Replay Takeover and Save States
This mod adds replay takeover functionality and training mode save states to the Steam release of Guilty Gear Xrd Rev 2. The mod is compatible with patch 2211 (current patch).
## Running the mod
- Grab the latest build from releases and unzip it.
- Open Guilty Gear Xrd.
- Run GGXrdReplayTakeoverInjector.exe

The next time you enter a replay/training mode the mod should be active. Using this method you will need to re-inject each time you wish to use the mod, see the next section if you would prefer to automatically inject the mod when you start the game.

If you are running the game on Linux via Proton then this direct injection method may not work due to restrictions on which processes can see each other. In this case it is recommended you instead use the automatic injection method.

## Automatic Injection
With a few more steps you can automatically enable the mod every time you run the game. This is the current recommended way of injecting with Linux/Proton to ensure the injection happens in a process that can access the Xrd process.

- Move the extracted GGXrdReplayTakeover.dll and GGXrdReplayTakeoverInjector.exe to Guilty Gear Xrd's binaries directory. The easiest way to find this is via the game's Steam Properties > Installed Files > Browse. From there you want to place the binaries in GUILTY GEAR Xrd -REVELATOR-/Binaries/Win32
- Next you need to edit the file BootGGXrd.bat in the GUILTY GEAR Xrd -REVELATOR- directory so that it runs the injector when the game starts. You can do so by adding the following line to the end of the file:
```GGXrdReplayTakeoverInjector.exe 10```

The 10 argument added to the injector tells it to wait 10 seconds before injecting. Normally an immediate injection is fine but sometimes the game is not fully running by the time the script runs the injector. This is usually the case on weaker hardware such as a steam deck, in which case the delay makes sure everything is ready by the time the injection happens.

If you would like to verify the injection is successful you can add another line to the batch file with the command ```pause```. This will prevent the console that runs the batch script from closing automatically after the game launches and you can confirm whether the output of the injector was successful or not.

Note that you don't need to worry about the mod interfering with online play or any other game modes. All the modifications to support the new features are only active in replays/training mode and are stripped out when you exit these modes.

## Using Replay Takeover
Enter a replay, you can confirm the mod is working as the replay control display will be replaced with the replay takeover controls and the pause menu will have extra options:

<img width="287" height="253" alt="Controls" src="https://github.com/user-attachments/assets/03daf915-207e-42a9-9c8e-5aacf3bc9715" />

<img width="521" height="316" alt="PauseMenu" src="https://github.com/user-attachments/assets/154a8b7a-1b31-420d-8ed7-43f258fae9d1" />

### Controls
All controls are based on your current bindings in the "Button Settings (Battle)" menu. This includes your bindings for record/play recording for training mode. The only exception is the select button (or your controller's equivalent) which cannot be rebound in any menu as far as I'm aware.

#### Replay Navigation
- P - pauses and unpause the replay.
- Left - Rewinds the replay
- Right - Advances the replay when paused. Does nothing when unpaused as the replay is already advancing at normal speed.
- Up/Down - Fast forwards/fast rewinds the replay 3 frames at a time.
- S/H - Step one frame back or forward. Only available while the replay is paused.

#### Replay Takeover Controls
- Play Recording - Initiates replay takeover on the current frame for the currently selected player. Or, if you are already in takeover, restarts the takeover.
- Record - Cancels a replay takeover. When takeover is cancelled you are returned to theframe it started with the game paused.

Additional controls for replay takeover are available from the pause menu:
- Takeover Player - The player you will control when you begin takeover. If you are already mid-takeover when changing this setting then it will take effect next time you restart the takeover.
- Takeover Countdown Frames - When you initiate or restart a takeover the game is paused for this many frames before you gain control of the player. Note that you can buffer inputs during this pause.

If either player would die during takeover, or the round would time out, the game will pause on the last frame before the round would end. This is allow you to restart or cancel takeover without being forced to go to the next round. If you have the control display on it will indicate "Round ended" when this happens:

<img width="1280" height="720" alt="RoundEndExample" src="https://github.com/user-attachments/assets/77577c5d-aa4f-4f87-b066-05cb728d50d8" />

#### Toggling Controls
- D - Toggles the control display.
- Select - Swaps to the normal non-mod replay controls or back to the mod controls. The main reason you'll probably want to do this is if you need to jump to the next round. But all other normal replay controls are also available here such as hiding the HUD/input history. Note - none of the mod's navigation or takeover functionality are available while you have swapped to the standard replay controls.

## Using Save States
There is currently no visual indicator for save states, just play about with the controls below to make sure they work. Note that save states do not persist when exiting and re-entering training mode as lots of save data is very specific to the game's current memory state.

### Controls
Save states uses contextual controls based on whether the SP/Special Move button is held. You probably won't have this button bound already since it's normally only used with stylish mode.

- Special Move + Record - Save State
- Special Move + Play - Load State

## Known Issues
- Cinematic supers that alter camera angle will display incorrectly while rewinding or fast forwarding a replay. Camera blending is currently very buggy with how rewinding works but it won't break any game logic.
- Cinematic supers that feature close ups of character's faces will also not show these cut ins correctly while navigating replays or loading save states. In these supers the game usually displays a special more detailed character model for the duration of the super freeze and the regular character actor gets hidden. This information is not properly perserved by save states atm and the more detailed model does not get displayed. This will make a lot of supers look very wack but won't break any game logic.
- Input history will display incorrectly while rewinding. Rewinding works by loading an older state then resimulating up to the desired frame, currently this results in the input history being filled with inputs from this resimulation.
- Several HUD elements display incorrectly when rewinding. For example, you may see duplicate counter hit messages and gauges like the eddie meter can get stuck on the wrong value. These errors are purely visual and do not affect the game logic. There is save state logic for handling these UI elements, but I currently have it disabled to improve stability as it was causing rare crashes during garbage collection.
- Loading a save state while Jack-O has placed house can sometimes cause a crash.
- Rewinding over a Bedman seal's destruction can sometimes cause a crash.
- Treasure hunted coins thrown by Johnny are invisible in loaded states/when rewinding. They require character specific initialisation that is not currently handled properly while loading states.

## Attributions
Many thanks to all the Xrd mods and related documentation created by Pangaea (@super-continent), @kkots and WorseThanYou. Their work was an amazing resource for learning how to mod Xrd which enabled me to create this.
