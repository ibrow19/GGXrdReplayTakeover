#include <training-controller.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <input.h>
#include <imgui.h>
#include <cstdio>

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
    AswEngine engine = XrdModule::GetEngine();
    if (!engine.IsValid() || 
        XrdModule::GetPreOrPostBattle() ||
        XrdModule::IsPauseMenuActive())
    {
        return;
    }

    // If the engine changes location from reloading training mode then none
    // of our save states are valid anymore.
    if (mEnginePtr == 0)
    {
        mEnginePtr = engine.GetPtr();
    }
    else if (mEnginePtr != engine.GetPtr())
    {
        mEnginePtr = engine.GetPtr();
        for (int i = 0; i < SaveStateCount; ++i)
        {
            mSaveStates[i].bValid = false;
        }
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
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
        ImGui::Text("Valid Save Data");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::Text("No Save Data");
        ImGui::PopStyleColor();
    }
    ImGui::End();
}
