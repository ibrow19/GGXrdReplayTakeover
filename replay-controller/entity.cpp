#include <entity.h>

Entity::Entity(DWORD inPtr)
: ptr(inPtr)
{}

bool Entity::IsPlayer()
{
    return *(DWORD*)(ptr + 0x10) == 1;
}
