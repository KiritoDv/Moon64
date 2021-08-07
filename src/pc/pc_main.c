#include <stdlib.h>
#include <stdio.h>

#include "sm64.h"

#include "game/memory.h"
#include "audio/external.h"

#include "gfx/gfx_pc.h"

#include "gfx/gfx_opengl.h"
#include "gfx/gfx_sdl.h"

#include "audio/audio_api.h"
#include "audio/audio_sdl.h"
#include "audio/audio_null.h"

#include "pc_main.h"
#include "cliopts.h"
#include "configfile.h"
#include "controller/controller_api.h"
#include "controller/controller_keyboard.h"
#include "fs/fs.h"

#include "game/game_init.h"
#include "game/main.h"
#include "game/thread6.h"

#ifdef DISCORDRPC
#include "pc/discord/discordrpc.h"
#endif

#ifdef TARGET_SWITCH
#include "nx/m_nx.h"
#endif

#include "text/text-loader.h"
#include "moon/moon64.h"

OSMesg D_80339BEC;
OSMesgQueue gSIEventMesgQueue;

s8 gResetTimer;
s8 D_8032C648;
s8 gDebugLevelSelect;
s8 gShowProfiler;
s8 gShowDebugText;

s32 gRumblePakPfs;
struct RumbleData gRumbleDataQueue[3];
struct StructSH8031D9B0 gCurrRumbleSettings;

static struct AudioAPI *audio_api;
static struct GfxWindowManagerAPI *wm_api;
static struct GfxRenderingAPI *rendering_api;

extern void gfx_run(Gfx *commands);
extern void thread5_game_loop(void *arg);
extern void create_next_audio_buffer(s16 *samples, u32 num_samples);
void game_loop_one_iteration(void);

void dispatch_audio_sptask(struct SPTask *spTask) {}
void set_vblank_handler(s32 index, struct VblankHandler *handler, OSMesgQueue *queue, OSMesg *msg) {}

static bool inited = false;

#include "game/display.h" // for gGlobalTimer
void send_display_list(struct SPTask *spTask) {
    if (!inited) return;
    gfx_run((Gfx *)spTask->task.t.data_ptr);
}

#define SAMPLES_HIGH 544
#define SAMPLES_LOW 528

void produce_one_frame(void) {
    moon_setup("Update");
    gfx_start_frame();

    const f32 master_mod = (f32)configMasterVolume / 127.0f;
    set_sequence_player_volume(SEQ_PLAYER_LEVEL, (f32)configMusicVolume / 127.0f * master_mod);
    set_sequence_player_volume(SEQ_PLAYER_SFX, (f32)configSfxVolume / 127.0f * master_mod);
    set_sequence_player_volume(SEQ_PLAYER_ENV, (f32)configEnvVolume / 127.0f * master_mod);

    game_loop_one_iteration();
    thread6_rumble_loop(NULL);
#ifdef TARGET_SWITCH
    controller_nx_rumble_loop();
#endif

    int samples_left = audio_api->buffered();
    u32 num_audio_samples = samples_left < audio_api->get_desired_buffered() ? SAMPLES_HIGH : SAMPLES_LOW;
    //printf("Audio samples: %d %u\n", samples_left, num_audio_samples);
    s16 audio_buffer[SAMPLES_HIGH * 2 * 2];
    for (int i = 0; i < 2; i++) {
        /*if (audio_cnt-- == 0) {
            audio_cnt = 2;
        }
        u32 num_audio_samples = audio_cnt < 2 ? 528 : 544;*/
        create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
    }
    //printf("Audio samples before submitting: %d\n", audio_api->buffered());

    audio_api->play((u8 *)audio_buffer, 2 * num_audio_samples * 4);

    gfx_end_frame();
}

void audio_shutdown(void) {
    if (audio_api) {
        if (audio_api->shutdown) audio_api->shutdown();
        audio_api = NULL;
    }
}

void game_deinit(void) {
#ifdef DISCORDRPC
    discord_shutdown();
#endif
    configfile_save(configfile_name());
    controller_shutdown();
    audio_shutdown();
    gfx_shutdown();
    moon_setup("Exit");
    inited = false;
#ifdef TARGET_SWITCH
    exitNX();
#endif
}

void game_exit(void) {
    game_deinit();
    exit(0);
}

void main_func(char *argv[]) {

    main_pool_init();
    gGfxAllocOnlyPool = alloc_only_pool_init();
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);

    const char *gamedir = gCLIOpts.GameDir[0] ? gCLIOpts.GameDir : FS_BASEDIR;
    const char *userpath = gCLIOpts.SavePath[0] ? gCLIOpts.SavePath : sys_user_path();
    fs_init(sys_ropaths, gamedir, userpath);
    moon_environment_save("MOON_CWD",   argv[0]);
    moon_environment_save("MOON_UPATH", userpath);
    moon_environment_save("ASSETS_DIR", gamedir);

    moon_setup("PreStartup");

    configfile_load(configfile_name());
    if (gCLIOpts.FullScreen == 1)
        configWindow.fullscreen = true;
    else if (gCLIOpts.FullScreen == 2)
        configWindow.fullscreen = false;

    wm_api = &gfx_sdl;
    rendering_api = &gfx_opengl_api;
    # ifdef USE_GLES
    #  define RAPI_NAME "OpenGL ES"
    # else
    #  define RAPI_NAME "OpenGL"
    # endif

    char window_title[96] =
    "Super Mario 64 - Moon64 (" RAPI_NAME ")";

    gfx_init(wm_api, rendering_api, window_title);
    wm_api->set_keyboard_callbacks(keyboard_on_key_down, keyboard_on_key_up, keyboard_on_all_keys_up);

    if (audio_api == NULL && audio_sdl.init())
        audio_api = &audio_sdl;

    if (audio_api == NULL) {
        audio_api = &audio_null;
    }

    audio_init();
    sound_init();

    thread5_game_loop(NULL);
    inited = true;

    fflush(stdout);
    moon_setup("PreInit");
    moon_setup("Init");

#ifdef DISCORDRPC
    discord_init();
#endif

    while (true) {
        wm_api->main_loop(produce_one_frame);
#ifdef DISCORDRPC
        discord_update_rich_presence();
#endif
    }
}

int main(int argc, char *argv[]) {
#ifdef TARGET_SWITCH
    initNX();
#endif
    parse_cli_opts(argc, argv);
    main_func(argv);
    return 0;
}
