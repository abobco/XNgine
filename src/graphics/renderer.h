#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"
#include "particles.h"
#include "../servers/message_queue.h"
#include "../lua-bindings/lua_util.h"
#include "../util/input.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

// #define MAX_BATCH_ELEMENTS  8192
#define DEFAULT_BATCH_BUFFER_ELEMENTS  1024

#define get_vert(vert_arr, i) (Vector3) {vert_arr[i], vert_arr[i+1], vert_arr[i+2]}

typedef struct PostProcessingFilter {
    RenderTexture2D render_target;
    Shader shader;
} PostProcessingFilter;

void draw_gamepad_state(GamepadNumber gp);

PostProcessingFilter* create_sobel_filter(int screen_width, int screen_height, const char *fs_file);

void draw_particle_texture(ParticleInfo particle, ParticleTexture pt, Color color);

void draw_scene(TerminalInfo *terminal);

void draw_particles(EmitterInfo* emitter);

void fill_tex(RenderTexture2D tex, Color color);

#ifdef __cplusplus
}
#endif