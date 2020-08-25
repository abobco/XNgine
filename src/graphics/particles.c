#include "particles.h"

#include "../util/bob_math.h"

ParticleInfo create_particle(Vector2 position, float mag, float scale) {
    ParticleInfo particle;

    particle.position = position;
    particle.velocity = (Vector2){ 
        (-1.0f + 2.0f*rand_float() )*mag,
        (-1.0f + 2.0f*rand_float() )*mag
    };
    particle.scale = scale;

    return particle;
}

EmitterInfo create_emitter(ParticleTexture texture, Color color, float start_speed,float d_speed, float max_scale, float d_scale ) {
    EmitterInfo emitter;

    emitter.position = (Vector2) {0,0};
    emitter.direction = (Vector2) {0,0};
    emitter.texture = texture;
    emitter.color = color;
    emitter.start_speed = fabsf(start_speed) > EPSILON ? start_speed : 1.0f;
    emitter.max_scale = fabsf(max_scale) > EPSILON ? max_scale : 1.0f;
    emitter.d_scale = d_scale;
    emitter.d_speed = d_speed;

    // emitter.particles[0] = (ParticleInfo) {*position_ptr, (Vector2) {5,0}, 0 };
    emitter.particles = calloc(MAX_PARTICLES, sizeof(ParticleInfo));
    emitter.num_particles = 0;

    return emitter;
}

void reset_emitter( EmitterInfo *e ) {
    e->num_particles = 0;
}

void update_particles(EmitterInfo *emitter, int n ) {

   if ( emitter->num_particles+n <= MAX_PARTICLES )
        emitter->num_particles+=n;

    // shift the particle array back
    for ( int i = emitter->num_particles-1-n; i >= 0; i-- )
        emitter->particles[i+n] = emitter->particles[i];

    Vector2 spawnPos = { 
        emitter->position.x, 
        emitter->position.y
    };
    
    for ( int i = 0; i < n; i++ ) {
        emitter->particles[i] = create_particle(spawnPos, emitter->start_speed, emitter->max_scale );  
    }

    // update particle positions
    for ( int i = 0; i < emitter->num_particles; i++ ) {
        ParticleInfo* p = &(emitter->particles[i]);

        p->position = add_vec2(p->position, p->velocity);

        p->velocity = lerp_vec2( (Vector2) {0,0}, p->velocity, emitter->d_speed);

        if ( p->scale > 0.2f )
            p->scale = lerp(0.0f, p->scale,  emitter->d_scale);
        else 
            p->scale = 0.0f; 
    }
}

ParticleTexture *create_particle_texture(int size, int shape, int i) {
    // ParticleTexture pt;

    trail_texture[i]._texture = LoadRenderTexture(size, size);
    trail_texture[i].shape = shape;
    trail_texture[i].size = size;
    
    BeginDrawing();
    BeginTextureMode(trail_texture[i]._texture);
        switch ( trail_texture[i].shape ) {
            case SHAPE_CIRCLE:
                DrawCircle(trail_texture[i].size/2, trail_texture[i].size/2, (float)trail_texture[i].size*0.5f, WHITE);
                break;
            case SHAPE_RECT:
                DrawRectangle(0, 0, trail_texture[i].size, trail_texture[i].size, WHITE);
                break;
            case SHAPE_TRI:
                DrawTriangle(
                    (Vector2) {trail_texture[i].size/2, 0},
                    (Vector2) {0, trail_texture[i].size},
                    (Vector2) {trail_texture[i].size, trail_texture[i].size},
                    WHITE
                );
                break;
        }
    EndTextureMode();
    EndDrawing();

    return &trail_texture[i];
}

void set_particle_texture(EmitterInfo *e, int i) {
    e->texture = *get_trail_texture(i);
}

ParticleTexture *get_trail_texture(int i) {return &trail_texture[i];}