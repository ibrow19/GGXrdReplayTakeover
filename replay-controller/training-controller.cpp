#include <training-controller.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <entity.h>
#include <input.h>
#include <cstdio>
#ifdef USE_IMGUI_OVERLAY
#include <imgui.h>
#endif

void TrainingController::InitMode()
{
    AttachSaveStateDetours();
    mEngineAddress = 0;
    mP1Char[0] = 0;
    mP2Char[0] = 0;
    mbValidSaveData = false;
    mbResetMode = false;
}

void TrainingController::ShutdownMode()
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

    // If the engine data moves or one of the characters change then any
    // existing save state is no longer valid. Ideally we should just reset
    // everything when training mode initalises but I'm not sure the best place
    // to hook onto for that so this is a decent work around for now.
    if (!mEngineAddress ||
        mEngineAddress != engine.GetPtr() ||
        strcmp(engine.GetP1Entity().GetCharacterCode(), mP1Char) ||
        strcmp(engine.GetP2Entity().GetCharacterCode(), mP2Char))
    {
        mEngineAddress = engine.GetPtr();
        memcpy(mP1Char, engine.GetP1Entity().GetCharacterCode(), CharCodeLen);
        memcpy(mP2Char, engine.GetP2Entity().GetCharacterCode(), CharCodeLen);
        mbValidSaveData = false;
        mbResetMode = false;
    }

    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battlePressed = input.GetP1BattleInput().GetPressedMask();
    DWORD& battleHeld = input.GetP1BattleInput().GetHeldMask();
    DWORD& menuPressed = input.GetP1MenuInput().GetPressedMask();
    bool bDirectionHeld = battleHeld & (DWORD)BattleInputMask::AllDirections;
    bool bResetPressed = menuPressed & (DWORD)MenuInputMask::Reset;
    if (bResetPressed && bDirectionHeld)
    {
        mbResetMode = false;
    } 
    else if (mbResetMode && bResetPressed && mbValidSaveData)
    {
        LoadState(mSaveData);
        mbResetMode = true;

        // Consume menu press to prevent reset being used for anything else.
        menuPressed = 0;
    }
    else if (battleHeld & (DWORD)BattleInputMask::Special)
    {
        if ((battlePressed & (DWORD)BattleInputMask::PlayRecording) && mbValidSaveData)
        {
            LoadState(mSaveData);
            mbResetMode = true;
        }
        else if (battlePressed & (DWORD)BattleInputMask::StartRecording)
        {
            SaveState(mSaveData);
            mbResetMode = false;
            mbValidSaveData = true;
        }

        // Consume all inputs while using contextual controls.
        battlePressed = 0;
    }
}

#ifdef USE_IMGUI_OVERLAY
void TrainingController::PrepareImGuiFrame()
{
    ImGui::Begin("Save States");
    ImGui::Text("Status: ");
    ImGui::SameLine();
    if (mbValidSaveData)
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

    if (ImGui::CollapsingHeader("Controls"))
    {
        ImGui::Text("Special Move + Play: Load State");
        ImGui::Text("Special Move + Record: Save State");
    }
    ImGui::End();
}
#endif
