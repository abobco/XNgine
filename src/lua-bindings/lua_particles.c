#include "lua_util.h"
#include "../graphics/renderer.h"

// server fns 
int lua_allocateParticleSystem( lua_State *L) {
    // create lua user data
    size_t nbytes = sizeof(EmitterInfo) + MAX_PARTICLES*sizeof(ParticleInfo); //  + (n)*sizeof(ParticleInfo)*MAX_PARTICLES;
    EmitterInfo *p = (EmitterInfo *) lua_newuserdata(L, nbytes);
    luaL_getmetatable(L, "Bob.ParticleSystem");
    lua_setmetatable(L, -2);
    Color col = lua_getColor(L, 1);
    float max_particle_speed = luaL_checknumber(L, 2);
    float d_speed = luaL_checknumber(L, 3);
    float max_scale = luaL_checknumber(L,4);
    float d_scale = luaL_checknumber(L, 5);
    *p = create_emitter(*get_trail_texture(0), col, max_particle_speed, d_speed, max_scale, d_scale);
    return 1;
}

static EmitterInfo *lua_checkParticleSystem( lua_State *L, int table_stack_idx ) {
    void *ud = luaL_checkudata(L, table_stack_idx, "Bob.ParticleSystem");
    luaL_argcheck(L, ud != NULL, table_stack_idx, "`ParticleSystem' expected");
    return (EmitterInfo *)ud;
}

int lua_resetEmitter(lua_State *L) {
    EmitterInfo *p = lua_checkParticleSystem(L,1);

    reset_emitter(p);

    return 0;
}

int lua_setEmitterPos( lua_State *L ) {
    EmitterInfo *p = lua_checkParticleSystem(L, 1);

    float x = luaL_checknumber(L,2);
    float y = luaL_checknumber(L,3);

    p->position.x = x;
    p->position.y = y;

    return 0;
}

int lua_getEmitterPos( lua_State *L ) {
    EmitterInfo *p = lua_checkParticleSystem(L, 1);

    lua_pushnumber(L, p->position.x);
    lua_pushnumber(L, p->position.y);
    
    return 2;
}

int lua_updateParticles( lua_State *L ) {
    EmitterInfo *p = lua_checkParticleSystem(L, 1);
    int n = luaL_checkinteger(L, 2);
    update_particles(p, n);
    return 0;
}

int lua_drawParticles( lua_State *L ) {
    EmitterInfo *p = lua_checkParticleSystem(L, 1);
    draw_particles( p );
    return 0;
}

int lua_openParticleLib(lua_State *L) {
    luaL_newmetatable(L, "Bob.ParticleSystem");
    
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // push the metatable
    lua_settable(L, -3); // metatable.__index = metatable
    luaL_setfuncs(L, lua_particles_m, 0);

    lua_newtable(L);
    luaL_setfuncs(L, lua_particles_f, 0);
    lua_setglobal(L, "particle_system");

    return 1;
}