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

SimpleActor::SimpleActor(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD SimpleActor::GetPtr()
{
    return mPtr;
}

DWORD& SimpleActor::GetTime()
{
    return *(DWORD*)(mPtr + 0x26c);
}
