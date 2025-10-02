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
    return *(DWORD*)(mPtr + 0x10) == 1;
}

DWORD Entity::GetSimpleActor()
{
    return *(DWORD*)(mPtr + 0x27cc);
}

DWORD Entity::GetComplexActor()
{
    return *(DWORD*)(mPtr + 0x27a8);
}

char* Entity::GetAnimationState()
{
    return (char*)(mPtr + 0xa58);
}
