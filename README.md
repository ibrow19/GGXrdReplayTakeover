# Guilty Gear Xrd Replay Takeover and Save States
This mod adds replay takeover functionality and training mode save states to the Steam release of Guilty Gear Xrd Rev 2. The mod is compatible with patch 2211 (current patch).
## Enabling the mod
- Grab the latest build from releases and unzip it.
- Open Guilty Gear Xrd.
- Run GGXrdReplayTakeoverInjector.exe

The next time you enter a replay/training mode the mod should be active. Using this method the mod will only be active for the current session and will need to be re-injected when you next wish to use it. See the next section if you would prefer to automatically inject the mod when you start the game.

If you are running the game on Linux via Proton then this direct injection method may not work due to restrictions on which processes can see each other. In this case it is recommended you instead use the automatic injection method described next.

## Automatic Injection
With a few more steps you can automatically enable the mod every time you run the game. This is the current recommended way of injecting with Linux/Proton to ensure the injection happens in a process that can access the Xrd process.

- Move the extracted GGXrdReplayTakeover.dll and GGXrdReplayTakeoverInjector.exe to Guilty Gear Xrd's binaries directory. The easiest way to find this is via the game's Steam Properties > Installed Files > Browse. From there you want to place the binaries in GUILTY GEAR Xrd -REVELATOR-/Binaries/Win32
- Next you need to edit the file BootGGXrd.bat in the GUILTY GEAR Xrd -REVELATOR- directory so that it runs the injector when the game starts. You can do so by adding the following line to the end of the file:
```GGXrdReplayTakeoverInjector.exe 10```

The 10 argument added to the injector tells it to wait 10 seconds before injecting. Often an immediate injection is fine but sometimes the game is not fully up and running by the time the script runs the injector in which case the delay is necessary.

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

After loading a state with Special Move + Play the behaviour of the Reset button will change to load that state for convenience instead of resetting to the default training mode position. The normal Reset behaviour can be restored by holding any direction while pressing reset.

## Known Issues
- Input history will display incorrectly while rewinding. Rewinding works by loading an older state then resimulating up to the desired frame, currently this results in the input history being filled with inputs from this resimulation.
- Several HUD elements display incorrectly when rewinding. For example, you may see duplicate counter hit messages and gauges like the eddie meter can get stuck on the wrong value. These errors are purely visual and do not affect the game logic. There is save state logic for handling these UI elements, but I currently have it disabled to improve stability as it was causing rare crashes during garbage collection.
- In some cases rewinding over a Bedman seal's destruction can cause a crash.
- Treasure hunted coins thrown by Johnny are invisible in loaded states/when rewinding. They require character specific initialisation that is not currently handled properly while loading states.

##  Compiling/Development
If you would like to compile the project yourself you will need the following requirements:
- Visual Studio 17 2022, MSVC >= version 19.43.34810 (Other versions are probably fine too, this is just what I know works)
- CMake >= version 3.16

To set up the project, first clone it along with the required submodules:
```
git clone --recurse-submodules https://github.com/ibrow19/GGXrdReplayTakeover.git
```
You will need to compile the detours library that the mod uses for hooking onto function calls. The easiest way to do this is to open the Visual Studio Command Prompt, navigate to the `GGXrdReplayTakeover/vendor/Detours/src` directory, and run `nmake`. If necessary you can find more details about building Detours here: https://github.com/microsoft/Detours/wiki/FAQ

Next, the repo contains some batch files to help generate project files and build the mod with its default configuration. Use `init_cmake.bat` from the `GGXrdReplayTakeover` directory to generate the project files. Notably this configures the project to be built for Win32, this is necessary to make the mod work with Xrd since it is a Win32 game.

After that you can run `build.bat` to compile the mod and the injector with CMake. By default this builds a Debug configuration of the project, you can change the `--config` argument in `build.bat` to Release if you need to build a release version. The project build files we generated earlier also produce a VS solution file so you can use Visual Studio to compile the project instead of using CMake via the batch file if you prefer.

### Injector differences between Release and Debug builds
There is a minor difference between how the injector behaves when compiling for release vs debug. When compiled for release the injector searches its current directory for the DLL to inject. When compiling for debug it instead search the path `build/Debug`. This is so that the injector can be more conveniently run from the project's root directory during development.

## Attributions
Many thanks to all the Xrd mods and related documentation created by Pangaea (@super-continent), @WistfulHopes, @kkots and WorseThanYou. Their work was an amazing resource for learning how to mod Xrd which enabled me to create this.
