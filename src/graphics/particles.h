#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"
#include <stdlib.h>

#define MAX_PARTICLES 100
#define PARTICLE_MAX_TICKS 60
#define PARTICLE_TEXTURE_SIZE 10
#define PARTICLE_SHRINK_RATE 0.05

// batched texture used to draw particles for one emitter
typedef struct ParticleTexture {
    RenderTexture2D _texture;
    int shape;      // shape options
    int size;       // pixel length of radius
} ParticleTexture;

enum SHAPES {
    SHAPE_CIRCLE,
    SHAPE_RECT,
    SHAPE_TRI
};

static ParticleTexture trail_texture[3];

typedef struct ParticleInfo {
    Vector2 position;
    Vector2 velocity;

    float scale;
} ParticleInfo;

typedef struct EmitterInfo {
    Vector2 position;      // parent player position
    Vector2 direction;     // parent player direction
    float start_speed;
    float max_scale;
    
    // rate of change
    float d_scale;
    float d_speed;

    ParticleTexture texture; // geometry texture generated at load time
    Color color;

    ParticleInfo* particles; // array of particles spawned by the emitter
    int num_particles;
} EmitterInfo;

typedef struct ParticleSystem {
    EmitterInfo* emitters;
    int num_emitters;
    int max_emitters;
} ParticleSystem;

ParticleInfo create_particle(Vector2 position, float mag, float max_scale);

EmitterInfo create_emitter(ParticleTexture texture, Color color, float start_speed, float d_speed, float start_scale, float d_scale);

void set_particle_texture(EmitterInfo *e, int i);

void update_particles(EmitterInfo *emitter, int n);

ParticleTexture *create_particle_texture(int size, int shape, int i);

ParticleTexture *get_trail_texture(int i);

void reset_emitter( EmitterInfo *e );

#ifdef __cplusplus
}
#endif