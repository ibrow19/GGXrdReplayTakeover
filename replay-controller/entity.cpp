#include <entity.h>

Entity::Entity(DWORD inPtr)
: mPtr(inPtr)
{}

bool Entity::IsPlayer()
{
    return *(DWORD*)(mPtr + 0x10) == 1;
}
