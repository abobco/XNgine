#include "lua_util.h"
#include <stdlib.h>

#include "../util/bob_math.h"

static XN_GameState* game_state;
static XN_SETTINGS *game_settings;
// --------------------------------------------------------------------------------------

XN_GameState *get_gamestate() { return game_state; }
XN_SETTINGS *get_settings() { return game_settings; }

int lua_setGameState(XN_GameState* state, XN_SETTINGS* s) {
    game_state = state;
    game_settings = s;
    return 0;
}

XN_GameState create_XN_GameState( XN_SETTINGS *settings) {
    XN_GameState newgame;

    newgame.settings = settings;
    return newgame;
}

int check_lua( lua_State *L, int r ){
    if (r != 0) {
        printf("[LUA] Error: %s\n", lua_tostring(L, -1));
        return true;
    }  
    return false;
}

void lua_check_script_function( lua_State *L, const char* function ) {
    lua_getglobal(L, function);
    if ( lua_isfunction( L, -1 ) ) {
        check_lua(L, lua_pcall(L, 0, 0, 0));
    }
}

/* create a global table with the given fns, 
   if the given libName is already taken, overwrites the old value */
void lua_openLib(lua_State *L, const struct luaL_Reg *fns, const char *libName) {
    lua_newtable(L);
    luaL_setfuncs(L, fns, 0);
    lua_setglobal(L, libName);
}

int lua_report_value_error(lua_State *L, int isValid, const char* var_name ) {
    if ( !isValid ) {
        printf("[LUA] Error: Invalid value '%s' for variable %s\n", lua_tostring(L, -1), var_name);
        return 0;
    }
    return 1;
}

int lua_checkOOB(lua_State *L, int i, int n) {
    luaL_argcheck(L, 1 <= i && i <= n, 2, "index out of range");
    return 0;
}

// general lua getter
lua_Number lua_getNumberFromTable( lua_State *L, int index, int table_stack_index ) {
    lua_pushnumber(L, index);
    lua_gettable(L, table_stack_index);
    lua_Number result = luaL_checknumber(L, -1); 
    lua_pop(L,1);
    return result;
} 

lua_Number lua_getNumberField( lua_State *L, const char *key, int table_stack_index) {
    lua_pushstring(L, key);
    lua_gettable(L, table_stack_index);
    lua_Number result = luaL_checknumber(L, -1); 
    lua_pop(L,1);
    return result;
}


int lua_checkField( lua_State *L, const char *key, int table_stack_index, int type) {
    lua_pushstring(L, key);
    if ( table_stack_index < 0 ) --table_stack_index;
    lua_gettable(L, table_stack_index);

    if ( lua_type(L, -1) != type ) {
         printf("[LUA] Error: Invalid type '%s' for variable %s. Expected '%s'\n", 
            lua_typename(L, lua_type(L, -1)), key, lua_typename(L, type));
        return -1;
    }
    return 0;
}

const char *lua_getStringField( lua_State *L, const char *key, int table_stack_index ) {
    lua_pushstring(L, key);
    if ( table_stack_index < 0 ) 
        --table_stack_index;
    lua_gettable(L, table_stack_index);
    size_t str_size;
    luaL_checklstring(L, -1, &str_size);
    const char *result = luaL_checkstring(L, -1);
    lua_pop(L,1);
    return result;
}

// These assume the table is at the top of the stack
void lua_setFloatField(lua_State *L, const char *key, float value) {
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);   
}
void lua_setIntField(lua_State *L, const char *key, int value) {
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);   
}

int lua_allocateFloatArray ( lua_State *L) {
    int n = luaL_checkinteger(L, 1);
    size_t nbytes = sizeof(FloatArray) + (n-1)*sizeof(float);
    FloatArray *a = (FloatArray*) lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, "Bob.FloatArray");
    lua_setmetatable(L, -2);

    a->size = n;
    return 1;   // new userdata is on the stack
}

static FloatArray *lua_checkFloatArray( lua_State *L ) {
    void *ud = luaL_checkudata(L, 1, "Bob.FloatArray");
    luaL_argcheck(L, ud != NULL, 1, "`array' expected");
    return (FloatArray *)ud;
}

static float *lua_getFloatElement( lua_State *L ) {
    FloatArray *a = lua_checkFloatArray(L);
    int idx = luaL_checkinteger(L, 2);

    luaL_argcheck(L, 1 <= idx && idx <= a->size, 2,
                "index out of range");

    // return element address
    return &a->buffer[idx - 1];
}
 
int lua_setFloatArray (lua_State *L) {
    float newvalue = luaL_checknumber(L,3);
    *lua_getFloatElement(L) = newvalue;
    return 0;
}

int lua_getFloatArray (lua_State *L) {
    lua_pushnumber(L, *lua_getFloatElement(L));
    return 1;
}

int lua_floatArraySize (lua_State *L) {
    FloatArray *a = (FloatArray *) lua_checkFloatArray(L);
    luaL_argcheck(L, a != NULL, 1, "`array' expected");
    lua_pushnumber(L, a->size);
    return 1;
}

int lua_floatArrayToString (lua_State *L) {
    FloatArray *a = lua_checkFloatArray(L);
    lua_pushfstring(L, "array(%d)", a->size);
    return 1;
}

// int lua_submitFloatArray( lua_State *L ) {
//     FloatArray *a = lua_checkFloatArray(L);
//     int id = luaL_checkinteger(L, 2);

//     XN_GameState *g = get_gamestate();
//     luaL_argcheck(L, 0 <= id && id < MAX_CONNECTIONS, 2,
//             "index out of range");

//     for ( int i =0; i < 8; i++ ) {
//         Vector2 v = {a->buffer[2*i], a->buffer[2*i+1]};
//         g->bomber_verts[id][i] = v;
//     }

//     return 0;
// }

int lua_openFloatArrayLib(lua_State *L) {
    luaL_newmetatable(L, "Bob.FloatArray");
    
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // push the metatable
    lua_settable(L, -3); // metatable.__index = metatable
    luaL_setfuncs(L, floatArrayLib_m, 0);

    lua_newtable(L);
    luaL_setfuncs(L, floatArrayLib_f, 0);
    lua_setglobal(L, "array");

    return 1;
}

int lua_getPerlinNoise(lua_State *L) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float f = luaL_checknumber(L, 3);
    float d = luaL_checkinteger(L, 4);

    float n = perlin2d(x,y,f,d);

    lua_pushnumber(L, n);

    return 1;
}

XN_SETTINGS load_settings(lua_State *L) {
    // load host libs onto lua VM
    lua_openLib(L, lua_raylib, "XN");
    lua_openFloatArrayLib(L);
    lua_openServerLib(L);
    lua_openParticleLib(L);
    lua_createColorTable(L);
    
    // load settings file
    struct XN_SETTINGS settings;
    check_lua(L, luaL_dofile(L, "../lua/settings.lua"));
    lua_getglobal(L, "XN_SETTINGS");
    if ( !lua_istable(L, -1) ) {
        printf("Not a valid settings table bro\n");
    }

    settings._LOG_LEVEL              = lua_checkIntField(L, "LOG_LEVEL", -1);
    settings._SCREEN_WIDTH           = lua_checkIntField(L, "SCREEN_WIDTH", -1);
    settings._SCREEN_HEIGHT          = lua_checkIntField(L, "SCREEN_HEIGHT", -1);

    return settings;
}