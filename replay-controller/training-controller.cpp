#include <training-controller.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <input.h>
#include <imgui.h>
#include <cstdio>

size_t TrainingController::mSelectedState = 0;
TrainingSaveState TrainingController::mSaveStates[SaveStateCount];

void TrainingController::AttachModeDetours()
{
    AttachSaveStateDetours();
}

void TrainingController::DetachModeDetours()
{
    DetachSaveStateDetours();
}

void TrainingController::Tick()
{
    if (!XrdModule::GetEngine().IsValid() || XrdModule::GetPreOrPostBattle())
    {
        return;
    }

    ApplySaveStateEntityUpdates();

    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battlePressed = input.GetP1BattleInput().GetPressedMask();
    DWORD& battleHeld = input.GetP1BattleInput().GetHeldMask();
    if (battleHeld & (DWORD)BattleInputMask::Special)
    {
        TrainingSaveState& state = mSaveStates[mSelectedState];
        if ((battlePressed & (DWORD)BattleInputMask::PlayRecording) &&
            state.bValid)
        {
            LoadState(state.saveData);
        }
        else if (battlePressed & (DWORD)BattleInputMask::StartRecording)
        {
            SaveState(state.saveData);
            state.bValid = true;
        }

        // Consume all inputs while using contextual controls.
        battlePressed = 0;
        battleHeld = 0;
    }
}

void TrainingController::PrepareImGuiFrame()
{
    ImGui::Begin("Save States");

    constexpr size_t maxLen = 3;
    char stringBuf[maxLen];
    snprintf(stringBuf, maxLen, "%d", mSelectedState+1);
    if (ImGui::BeginCombo("State", stringBuf))
    {
        for (size_t i = 0; i < SaveStateCount; ++i)
        {
            snprintf(stringBuf, maxLen, "%d", i+1);
            if (ImGui::Selectable(stringBuf))
            {
                mSelectedState = i;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Status: ");
    ImGui::SameLine();
    if (mSaveStates[mSelectedState].bValid)
    {
        ImGui::Text("Valid Save Data");
    }
    else
    {
        ImGui::Text("No Save Data");
    }
    ImGui::End();
}
