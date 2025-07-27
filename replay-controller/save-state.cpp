#include <common.h>
#include <save-state.h>

// Save state is comprised 
static const SaveStateSection StateSections[17] =
{
    // 0 - main engine chunk
    { 
        0x9e20a0,
        0x9e2320,
        AswEngine,
        0x4
    },
    // 1 - Unknown
    {
        0xbad880,
        0xbad910,
        AswEngine,
        0x1c71f4
    },
    // 2 - Unknown
    {
        0xb64de0,
        0xb64e70,
        AswEngine,
        0x1c6f7c
    },
    // 3 - Unknown
    {
        0x9741d0,
        0x974270,
        Module,
        0x1738900
    },
    // 4 - Unknown
    {
        0xc0a460,
        0xc0a730,
        Module,
        0x198b7d8
    },
    // 5 - Unknown
    {
        0xc0ae70,
        0xc0aed0,
        Module,
        0x198b7e4
        
    },
    // 6 - Unknown
    {
        0xc05cf0,
        0xc060a0,
        Module,
        0x198b800 
    },
    // 7 - Unknown
    {
        0xc0b0c0,
        0xc0b320,
        Module,
        0x198b7f0 
    },
    // 8 - Unknown
    {
        0xc0b010,
        0xc0b060,
        Module,
        0x198b7f8
    },
    // 9 - Unknown
    {
        0x974f20,
        0x974fc0,
        Module,
        0x158ffe8
    },
    // 10 - Unknown
    {
        0x9c73e0,
        0x9c7490,
        AswEngine,
        0x1c7110
        
    },
    // 11 - Unknown
    {
        0x9de460,
        0x9de4f0,
        AswEngine,
        0x1c7430
        
    },
    // 12 - Unknown
    {
        0xc118e0,
        0xc119e0,
        Module,
        0x1737eb8
        
    },
    // 13 - Unknown
    {
        0xc19fe0,
        0xc1a070,
        Module,
        0x1b1da8c
        
    },
    // 14 - Unknown
    {
        0xbfdca0,
        0xbfdd30,
        Module,
        0x198b808
    },
    // 15 - Unknown
    {
        0xc0b5d0,
        0xc0ba80,
        Module,
        0x198b810
        
        
    },
    // 16 - Unknown
    {
        0xbc9140,
        0xbc91d0,
        AswEngine,
        0x1c73fc
        
    },
};


void DetourSaveStateFunctions()
{
    char* xrdOffset = GetModuleOffset(GGameName);
}
