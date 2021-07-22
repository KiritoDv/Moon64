#include <PR/ultratypes.h>

#include "engine/math_util.h"
#include "game/memory.h"
#include "game/segment2.h"
#include "game/segment7.h"
#include "intro_geo.h"
#include "sm64.h"
#include "textures.h"
#include "types.h"
#include "buffers/framebuffers.h"
#include "game/game_init.h"
#include "audio/external.h"
#include "prevent_bss_reordering.h"
#include "gfx_dimensions.h"
#include "engine/graph_node.h"

// frame counts for the zoom in, hold, and zoom out of title model
#define INTRO_STEPS_ZOOM_IN 20
#define INTRO_STEPS_HOLD_1 75
#define INTRO_STEPS_ZOOM_OUT 91

// background types
#define INTRO_BACKGROUND_SUPER_MARIO 0
#define INTRO_BACKGROUND_GAME_OVER 1

struct GraphNodeMore {
    /*0x00*/ struct GraphNode node;
    /*0x14*/ void *todo;
    /*0x18*/ u32 unk18;
};

// intro geo bss
static u16 *sFrameBuffers[3];
static s32 sGameOverFrameCounter;
static s32 sGameOverTableIndex;
static s16 sIntroFrameCounter;
static s32 sTmCopyrightAlpha;

float gGlobalCounter = 0;

s32 gTitlePingPong = 0;
float gTitlePos = 0;
float gTitleNewPos = 0;

s32 gTitleRotationCounter = 0;

// intro screen background texture X offsets
float introBackgroundOffsetX[] = {
    0.0, 80.0, 160.0, 240.0, 0.0, 80.0, 160.0, 240.0, 0.0, 80.0, 160.0, 240.0,
};

// intro screen background texture Y offsets
float introBackgroundOffsetY[] = {
    160.0, 160.0, 160.0, 160.0, 80.0, 80.0, 80.0, 80.0, 0.0, 0.0, 0.0, 0.0,
};

// table that points to either the "Super Mario 64" or "Game Over" tables
const u8 *const *introBackgroundTextureType[] = { mario_title_texture_table, game_over_texture_table };

s8 introBackgroundIndexTable[] = {
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
};

// only one table of indexes listed
s8 *introBackgroundTables[] = { introBackgroundIndexTable };

s8 gameOverBackgroundTable[] = {
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
};

// order of tiles that are flipped from "Game Over" to "Super Mario 64"
s8 gameOverBackgroundFlipOrder[] = { 0x00, 0x01, 0x02, 0x03, 0x07, 0x0B,
                                     0x0a, 0x09, 0x08, 0x04, 0x05, 0x06 };



Gfx *geo_n64_screen(s32 sp50, struct GraphNode *sp54, UNUSED void *context) {
    struct GraphNode *graphNode; // sp4c
    Gfx *displayList;            // sp48
    Gfx *displayListIter;        // sp44

    Mtx *mtxDest;
    Mtx *scaleMat;
    Mtx *rotMat;
    Mtx *transMat;

    f32 *scaleTable1;            // sp3c
    f32 *scaleTable2;            // sp38
    f32 scaleX;                  // sp34
    f32 scaleY;                  // sp30
    f32 scaleZ;                  // sp2c
    graphNode = sp54;
    displayList = NULL;
    displayListIter = NULL;
    scaleTable1 = segmented_to_virtual(intro_seg7_table_0700C790);
    scaleTable2 = segmented_to_virtual(intro_seg7_table_0700C880);
    graphNode->flags = (graphNode->flags & 0xFF) | 0x100;

    mtxDest = alloc_display_list(3 * sizeof(*mtxDest));
    scaleMat = alloc_display_list(3 * sizeof(*scaleMat));
    rotMat = alloc_display_list(3 * sizeof(*rotMat));
    transMat = alloc_display_list(3 * sizeof(*transMat));

    displayList = alloc_display_list(5 * sizeof(*displayList));
    displayListIter = displayList;
    scaleX = 1.0f;
    scaleY = 1.0f;
    scaleZ = 1.0f;
    guScale(scaleMat, 0, scaleY, scaleZ);
    guRotate(scaleMat, 10, 1, 0, 0);
    guRotate(rotMat, gTitleRotationCounter, 0, 1, 0);
    //guTranslate(transMat, 0, -700 + gTitleTransCounter, 0);
    guTranslate(transMat, 0, gTitlePos, 0);

    mtxf_mul(mtxDest, scaleMat, rotMat);
    mtxf_mul(mtxDest, mtxDest, transMat);

    gSPMatrix(displayListIter++, mtxDest, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(displayListIter++, &n64logo_N64_mesh);
    gSPPopMatrix(displayListIter++, G_MTX_MODELVIEW);
    gSPEndDisplayList(displayListIter);
    gGlobalCounter += 1.0f;

    int range = 80;
    float step = 1;

    if(gTitlePos >= range)
        gTitlePingPong = 1;
    else if (gTitlePos <= -range)
        gTitlePingPong = 0;

    gTitleNewPos += step * (gTitlePingPong ? -1 : 1);
    gTitlePos = lerp(gTitlePos, gTitleNewPos, (gGlobalCounter / 1000) * 100);

    gTitleRotationCounter += 2;

    return displayList;
}


Gfx *geo_title_screen(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNode *graphNode = node;
    Gfx *dl = NULL;
    Gfx *dlIter = NULL;
    Mtx *scaleMat;
    f32 *scaleTable1 = segmented_to_virtual(intro_seg7_table_0700C790);
    f32 *scaleTable2 = segmented_to_virtual(intro_seg7_table_0700C880);
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    if (state != 1) {
        sIntroFrameCounter = 0;
    } else if (state == 1) {
        graphNode->flags = (graphNode->flags & 0xFF) | (LAYER_OPAQUE << 8);
        scaleMat = alloc_display_list(sizeof(*scaleMat));
        dl = alloc_display_list(4 * sizeof(*dl));
        dlIter = dl;

        // determine scale based on the frame counter
        if (sIntroFrameCounter >= 0 && sIntroFrameCounter < INTRO_STEPS_ZOOM_IN) {
            // zooming in
            scaleX = scaleTable1[sIntroFrameCounter * 3];
            scaleY = scaleTable1[sIntroFrameCounter * 3 + 1];
            scaleZ = scaleTable1[sIntroFrameCounter * 3 + 2];
        } else if (sIntroFrameCounter >= INTRO_STEPS_ZOOM_IN && sIntroFrameCounter < INTRO_STEPS_HOLD_1) {
            // holding
            scaleX = 1.0f;
            scaleY = 1.0f;
            scaleZ = 1.0f;
        } else if (sIntroFrameCounter >= INTRO_STEPS_HOLD_1 && sIntroFrameCounter < INTRO_STEPS_ZOOM_OUT) {
            // zooming out
            scaleX = scaleTable2[(sIntroFrameCounter - INTRO_STEPS_HOLD_1) * 3];
            scaleY = scaleTable2[(sIntroFrameCounter - INTRO_STEPS_HOLD_1) * 3 + 1];
            scaleZ = scaleTable2[(sIntroFrameCounter - INTRO_STEPS_HOLD_1) * 3 + 2];
        } else {
            // disappeared
            scaleX = 0.0f;
            scaleY = 0.0f;
            scaleZ = 0.0f;
        }
        guScale(scaleMat, scaleX, scaleY, scaleZ);

        gSPMatrix(dlIter++, scaleMat, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPDisplayList(dlIter++, &intro_seg7_dl_0700B3A0);  // draw model
        gSPPopMatrix(dlIter++, G_MTX_MODELVIEW);
        gSPEndDisplayList(dlIter);

        sIntroFrameCounter++;
    }
    return dl;
}

Gfx *geo_fade_transition(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNode *graphNode = node;
    Gfx *dl = NULL;
    Gfx *dlIter = NULL;

    if (state != 1) {  // reset
        sTmCopyrightAlpha = 0;
    } else if (state == 1) {  // draw
        dl = alloc_display_list(5 * sizeof(*dl));
        dlIter = dl;
        gSPDisplayList(dlIter++, dl_proj_mtx_fullscreen);
        gDPSetEnvColor(dlIter++, 255, 255, 255, sTmCopyrightAlpha);
        switch (sTmCopyrightAlpha) {
            case 255: // opaque
                graphNode->flags = (graphNode->flags & 0xFF) | (LAYER_OPAQUE << 8);
                gDPSetRenderMode(dlIter++, G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2);
                break;
            default: // blend
                graphNode->flags = (graphNode->flags & 0xFF) | (LAYER_TRANSPARENT << 8);
                gDPSetRenderMode(dlIter++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
                break;
        }
        gSPDisplayList(dlIter++, &intro_seg7_dl_0700C6A0);  // draw model
        gSPEndDisplayList(dlIter);

        // Once the "Super Mario 64" logo has just about zoomed fully, fade in the "TM" and copyright text
        if (sIntroFrameCounter >= 19) {
            sTmCopyrightAlpha += 26;
            if (sTmCopyrightAlpha > 255) {
                sTmCopyrightAlpha = 255;
            }
        }
    }
    return dl;
}

Gfx *intro_backdrop_set_image(f32 x, f32 y, s8 index) {
    // intro screen background display lists for each of four 80x20 textures
    static const Gfx *introBackgroundDlRows[] = { title_screen_bg_dl_0A000130, title_screen_bg_dl_0A000148,
                                                  title_screen_bg_dl_0A000160, title_screen_bg_dl_0A000178 };

    // table that points to either the "Super Mario 64" or "Game Over" tables
    static const u8 *const *textureTables[] = { mario_title_texture_table, game_over_texture_table };

    Mtx *mtx;
    Gfx *displayList;
    Gfx *displayListIter;
    const u8 *const *vIntroBgTable;
    s32 i;
    mtx = alloc_display_list(sizeof(*mtx));
    displayList = alloc_display_list(36 * sizeof(*displayList));
    displayListIter = displayList;
    vIntroBgTable = segmented_to_virtual(textureTables[index]);
    guTranslate(mtx, x, y, 0.0f);
    gSPMatrix(displayListIter++, mtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000118);
    for (i = 0; i < 4; ++i) {
        gDPLoadTextureBlock(displayListIter++, vIntroBgTable[i], G_IM_FMT_RGBA, G_IM_SIZ_16b, 80, 20, 0,
                            G_TX_CLAMP, G_TX_CLAMP, 7, 6, G_TX_NOLOD, G_TX_NOLOD)
        gSPDisplayList(displayListIter++, introBackgroundDlRows[i]);
    }
    gSPPopMatrix(displayListIter++, G_MTX_MODELVIEW);
    gSPEndDisplayList(displayListIter);
    return displayList;
}

Gfx *geo_intro_backdrop(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNodeMore *graphNode;
    Gfx *displayList;
    Gfx *displayListIter;
    s32 w, x, y;

    // Get how many horizontal tiles to draw (limit width to prevent segfault)
    w = ((SCREEN_HEIGHT * GFX_DIMENSIONS_ASPECT_RATIO) + 79) / 80;
    if (w > 50)
        w = 50;

    graphNode = (struct GraphNodeMore *) node;
    displayList = NULL;
    displayListIter = NULL;
    if (state == 1) {
        displayList = alloc_display_list((4 + w * 3) * sizeof(*displayList));
        displayListIter = displayList;
        graphNode->node.flags = (graphNode->node.flags & 0xFF) | (LAYER_OPAQUE << 8);
        gSPDisplayList(displayListIter++, &dl_proj_mtx_fullscreen);
        gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000100);
        for (y = 0; y < 3; ++y) {
            for (x = 0; x < w; ++x) {
                gSPDisplayList(displayListIter++, intro_backdrop_set_image(80.0f * x + SCREEN_WIDTH / 2 - w * 40.0f, 160.0f - 80.0f * y, INTRO_BACKGROUND_SUPER_MARIO));
            }
        }
        gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000190);
        gSPEndDisplayList(displayListIter);
    }
    return displayList;
}

Gfx *geo_game_over_tile(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNodeMore *graphNode;
    Gfx *displayList;
    Gfx *displayListIter;
    s32 w, x, y;
    s8 index;

    // Get how many horizontal tiles to draw (limit width to prevent segfault)
    w = ((SCREEN_HEIGHT * GFX_DIMENSIONS_ASPECT_RATIO) + 79) / 80;
    if (w > 50)
        w = 50;

    graphNode = (struct GraphNodeMore *) node;
    displayList = NULL;
    displayListIter = NULL;
    if (state != 1) {
        sGameOverFrameCounter = 0;
        sGameOverTableIndex = -1;
    } else {
        displayList = alloc_display_list((4 + w * 3) * sizeof(*displayList));
        displayListIter = displayList;

        // Wait 180 frames, then increment flipped tile index
        if (sGameOverTableIndex == -1) {
            if (sGameOverFrameCounter == 180) {
                sGameOverTableIndex++;
                sGameOverFrameCounter = 0;
            } else {
                sGameOverFrameCounter++;
            }
        }
        else
        {
            if ((++sGameOverFrameCounter & 1) == 0) {
                if (sGameOverTableIndex < 0x7FFFFFFF) {
                    sGameOverTableIndex++;
                }
            }
        }

        graphNode->node.flags = (graphNode->node.flags & 0xFF) | (LAYER_OPAQUE << 8);
        gSPDisplayList(displayListIter++, &dl_proj_mtx_fullscreen);
        gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000100);
        for (y = 0; y < 3; ++y) {
            for (x = 0; x < w; ++x) {
                // Get if this tile should be a "Game Over" or "Super Mario 64" tile based off of the current flip index
                index = INTRO_BACKGROUND_GAME_OVER;
                switch (y)
                {
                    case 2:
                        if (x > w - (sGameOverTableIndex - w)) {
                            index = INTRO_BACKGROUND_SUPER_MARIO;
                        }
                        break;
                    case 1:
                        if ((x == w - 1 && sGameOverTableIndex > w) || (x < (sGameOverTableIndex - w * 2 - 1))) {
                            index = INTRO_BACKGROUND_SUPER_MARIO;
                        }
                        break;
                    case 0:
                        if (x < sGameOverTableIndex) {
                            index = INTRO_BACKGROUND_SUPER_MARIO;
                        }
                        break;
                }

                gSPDisplayList(displayListIter++, intro_backdrop_set_image(80.0f * x + SCREEN_WIDTH / 2 - w * 40.0f, 160.0f - 80.0f * y, index));
            }
        }
        gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000190);
        gSPEndDisplayList(displayListIter);
    }
    return displayList;
}

extern Gfx title_screen_bg_dl_0A0065E8[];
extern Gfx title_screen_bg_dl_0A006618[];

//Data
s8 sFaceVisible[] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
};

s8 sFaceToggleOrder[] = {
     0,  1,  2,  3,  4,  5,  6,  7,
    15, 23, 31, 39, 47, 46, 45, 44,
    43, 42, 41, 40, 32, 24, 16,  8,
     9, 10, 11, 12, 13, 14, 22, 30,
    38, 37, 36, 35, 34, 33, 25, 17,
};

s8 sFaceCounter = 0;

void intro_gen_face_texrect(Gfx **dlIter) {
    s32 x;
    s32 y;

    for (y = 0; y < 6; y++) {
        for (x = 0; x < 8; x++) {
            if (sFaceVisible[y*8 + x] != 0) {
                gSPTextureRectangle((*dlIter)++, (x * 40) << 2, (y * 40) << 2, (x * 40 + 39) << 2, (y * 40 + 39) << 2, 0,
                                    0, 0, 4 << 10, 1 << 10);
            }
        }
    }
}

Gfx *intro_draw_face(u16 *image, s32 imageW, s32 imageH) {
    Gfx *dl;
    Gfx *dlIter;

    // ex-alo change
    // extend number to 130 to fix rendering
    dl = alloc_display_list(130 * sizeof(Gfx));

    if (dl == NULL) {
        return dl;
    } else {
        dlIter = dl;
    }

    gSPDisplayList(dlIter++, title_screen_bg_dl_0A0065E8);

    gDPLoadTextureBlock(dlIter++, image, G_IM_FMT_FRAMEBUFFER, G_IM_SIZ_16b, imageW, imageH, 0,
        G_TX_CLAMP | G_TX_NOMIRROR, G_TX_CLAMP | G_TX_NOMIRROR, 6, 6, G_TX_NOLOD, G_TX_NOLOD);

    intro_gen_face_texrect(&dlIter);

    gSPDisplayList(dlIter++, title_screen_bg_dl_0A006618);

    gSPEndDisplayList(dlIter++);

    return dl;
}

u16 *intro_sample_frame_buffer(s32 imageW, s32 imageH, s32 sampleW, s32 sampleH) {
    u16 *fb;
    u16 *image;
    s32 pixel;
    f32 size;
    f32 r, g, b;
    s32 iy, ix, sy, sx;

    s32 xOffset = 120;
    s32 yOffset = 80;

    fb = get_framebuffer();

    image = malloc(imageW * imageH * sizeof(u16));

    if (image == NULL) {
        return image;
    }

    for (iy = 0; iy < imageH; iy++) {
        for (ix = 0; ix < imageW; ix++) {
            r = 0;
            g = 0;
            b = 0;

            for (sy = 0; sy < sampleH; sy++) {
                for (sx = 0; sx < sampleW; sx++) {
                    u16 fbr, fbg, fbb;
                    f32 f1, f2, f3;
                    pixel = 320 * (sampleH * iy + sy + yOffset) + (sampleW * ix + xOffset) + sx;

                    fbr = fb[pixel];
                    fbg = fb[pixel];
                    fbb = fb[pixel];

                    f1 = ((fbr >> 0xB) & 0x1F);
                    f2 = ((fbg >> 0x6) & 0x1F);
                    f3 = ((fbb >> 0x1) & 0x1F);

                    r += f1;
                    g += f2;
                    b += f3;
                }
            }

            size = sampleW * sampleH;
            u16 color = ((((u16) (r / size + 0.5) << 0xB) & 0xF800) & 0xffff) +
                        ((((u16) (g / size + 0.5) << 0x6) &  0x7C0) & 0xffff) +
                        ((((u16) (b / size + 0.5) << 0x1) &   0x3E) & 0xffff) + 1;

            // Endian swap
            image[imageH * iy + ix] = (color << 8) | (color >> 8);
        }
    }

    return image;
}

Gfx *geo_intro_face_easter_egg(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNodeGenerated *genNode = (struct GraphNodeGenerated *)node;
    u16 *image;
    Gfx *dl = NULL;
    s32 i;

    if (state != 1) {
        sFrameBuffers[0] = gFrameBuffer0;
        sFrameBuffers[1] = gFrameBuffer1;
        sFrameBuffers[2] = gFrameBuffer2;

        for (i = 0; i < 48; i++) {
            sFaceVisible[i] = 0;
        }

    } else if (state == 1) {
        if (sFaceCounter == 0) {
            if (gPlayer1Controller->buttonPressed & R_TRIG) {
                play_sound(SOUND_MENU_STAR_SOUND, gGlobalSoundSource);
                sFaceVisible[0] ^= 1;
                sFaceCounter++;
            }
        } else {
            sFaceVisible[sFaceToggleOrder[sFaceCounter++]] ^= 1;
            if (sFaceCounter >= 40) {
                sFaceCounter = 0;
            }
        }

        // Draw while the first or last face is visible.
        if (sFaceVisible[0] == 1 || sFaceVisible[17] == 1) {
            image = intro_sample_frame_buffer(40, 40, 2, 2);
            if (image != NULL) {
                genNode->fnNode.node.flags = (genNode->fnNode.node.flags & 0xFF) | (LAYER_OPAQUE << 8);
                dl = intro_draw_face(image, 40, 40);
            }
        }
    }

    return dl;
}

extern Gfx title_screen_bg_dl_0A007548_start[];
extern Gfx title_screen_bg_dl_0A007548_end[];

Gfx *geo_intro_rumble_pak_graphic(s32 state, struct GraphNode *node, UNUSED void *context) {
    struct GraphNodeGenerated *genNode = (struct GraphNodeGenerated *)node;
    Gfx *dlIter;
    Gfx *dl;
    s32 introContext;
    s8 backgroundTileSix;
    u16 left;
    dl = NULL;
    backgroundTileSix = 0;

    left = GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(100);

    if (state != 1) {
        dl = NULL;
    } else if (state == 1) {
        genNode->fnNode.node.flags = (genNode->fnNode.node.flags & 0xFF) | (LAYER_OPAQUE << 8);
        introContext = genNode->parameter & 0xFF;
        if (introContext == 0) {
            backgroundTileSix = INTRO_BACKGROUND_SUPER_MARIO;
        } else if (introContext == 1) {
            if (sGameOverTableIndex <= 0) {
                backgroundTileSix = INTRO_BACKGROUND_GAME_OVER;
            } else {
                backgroundTileSix = INTRO_BACKGROUND_SUPER_MARIO;
            }
        }
        if (backgroundTileSix == INTRO_BACKGROUND_SUPER_MARIO) {
            dl = alloc_display_list(6 * sizeof(*dl));
            if (dl != NULL) {
                dlIter = dl;
                gSPDisplayList(dlIter++, &title_screen_bg_dl_0A007548_start);
                gSPTextureRectangle(dlIter++, left << 2, 200 << 2, (left + 79) << 2, (200 + 23) << 2, 7, 0, 0, 4 << 10, 1 << 10);
                gSPDisplayList(dlIter++, &title_screen_bg_dl_0A007548_end);
                gSPEndDisplayList(dlIter);
            }
        } else {
            dl = NULL;
        }
    }
    return dl;
}