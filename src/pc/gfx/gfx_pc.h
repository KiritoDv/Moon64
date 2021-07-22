#ifndef GFX_PC_H
#define GFX_PC_H

#include <PR/gbi.h>

struct GfxRenderingAPI;
struct GfxWindowManagerAPI;

struct GfxDimensions {
    uint32_t width, height;
    float aspect_ratio;
};

#define MAX_BUFFERED_TRIANGLES 256
#define VERTEX_BUFFER_SIZE MAX_BUFFERED_TRIANGLES * 26 * 3 // 3 vertices in a triangle and 26 floats per vtx

#define MAX_CACHED_TEXTURES 4096 // for preloading purposes
#define HASH_SHIFT 0

#define HASHMAP_LEN (MAX_CACHED_TEXTURES * 2)
#define HASH_MASK (HASHMAP_LEN - 1)

struct TextureData {
    const uint8_t *texture_addr;
    uint8_t fmt, siz;

    uint32_t texture_id;
    uint8_t cms, cmt;
    char linear_filter;
};

extern struct GfxDimensions gfx_current_dimensions;

#ifdef __cplusplus
extern "C" {
#endif

void gfx_init(struct GfxWindowManagerAPI *wapi, struct GfxRenderingAPI *rapi, const char *window_title);
struct GfxRenderingAPI *gfx_get_current_rendering_api(void);
uint16_t *get_framebuffer();
void gfx_start_frame(void);
void gfx_run(Gfx *commands);
void gfx_end_frame(void);
void gfx_shutdown(void);
void overload_memory_texture(void* data, long size, const char *path);
struct TextureData * forceTextureLoad(char* path);
#ifdef __cplusplus
}
#endif

#endif
