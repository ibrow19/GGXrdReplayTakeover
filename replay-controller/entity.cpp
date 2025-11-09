#include <entity.h>

Entity::Entity(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD Entity::GetPtr()
{
    return mPtr;
}

bool Entity::IsPlayer()
{
    return *(DWORD*)(mPtr + 0x10) != 0;
}

char* Entity::GetCharacterCode()
{
    return (char*)(mPtr + 0x27ac);
}

DWORD& Entity::GetSimpleActor()
{
    return *(DWORD*)(mPtr + 0x27cc);
}

DWORD& Entity::GetSimpleActorSaveData()
{
    return *(DWORD*)(mPtr + 0x2918);
}

DWORD& Entity::GetComplexActor()
{
    return *(DWORD*)(mPtr + 0x27a8);
}

char* Entity::GetStateName()
{
    return (char*)(mPtr + 0x2858);
}

char* Entity::GetAnimationFrameName()
{
    return (char*)(mPtr + 0xa58);
}

bool Entity::HasNonPlayerComplexActorOnline()
{
    return *(DWORD*)(mPtr + 0x2878) != 0;
}

DWORD& Entity::GetComplexActorRecreationFlag()
{
    return *(DWORD*)(mPtr + 0x287c);
}

TimeStepData::TimeStepData(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD TimeStepData::GetPtr()
{
    return mPtr;
}

bool TimeStepData::IsValid()
{
    return mPtr != 0;
}

bool TimeStepData::ShouldUseFixedTimeStep()
{
    return *(BYTE*)(mPtr + 0x3c) == 1;
}

float& TimeStepData::GetFixedTimeStep()
{
    return *(float*)(mPtr + 0x44);
}

SimpleActor::SimpleActor(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD SimpleActor::GetPtr()
{
    return mPtr;
}

float& SimpleActor::GetTime()
{
    return *(float*)(mPtr + 0x26c);
}

float& SimpleActor::GetDeltaCoeff()
{
    return *(float*)(mPtr + 0x2d8);
}

TimeStepData SimpleActor::GetTimeStepData()
{
    return TimeStepData(*(DWORD*)(mPtr + 0x1d8));
}
