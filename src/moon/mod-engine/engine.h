#ifndef Moon64ModEngine
#define Moon64ModEngine

#include "moon/mod-engine/interfaces/bit-module.h"
#include <vector>

extern "C" {
#include "pc/gfx/gfx_pc.h"
}

namespace Moon {
    extern std::vector<BitModule*> addons;
    void loadAddon(std::string addonPath);
}

void Moon_SaveTexture(TextureData* data, std::string tex);
TextureData* Moon_GetTexture(std::string texture);
void Moon_PreInitModEngine();
void Moon_InitModEngine( char *exePath, char *gamedir );

void Moon_LoadBaseTexture(char* data, long size, std::string texture);


// TESTS
void Moon_TextFlyLoad(int id);
void Moon_TestRebuildOrder(std::vector<int> order);
void Moon_LoadTexture(int tile, const char *fullpath, struct GfxRenderingAPI *gfx_rapi);

#endif