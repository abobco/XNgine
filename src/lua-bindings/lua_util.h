#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "raylib.h"

#include "../../build/_deps/lua-5.4.0/install/include/lua.h"
#include "../../build/_deps/lua-5.4.0/install/include/lauxlib.h"
#include "../../build/_deps/lua-5.4.0/install/include/lualib.h"
#include "../graphics/particles.h"
#include "../util/bob_math.h"

#define PRINT(a) printf(#a " = %d\n", a)
#define print(a) printf(#a " = %d\n", a)

typedef struct XN_SETTINGS {
    // debug log option
    int _LOG_LEVEL;
    
    // scene
    int _SCREEN_WIDTH;
    int _SCREEN_HEIGHT;

    int _WEBSOCKET_PORT;
    char _WEBSOCKET_DOMAIN[124];
} XN_SETTINGS;

typedef struct OBB {
    Vector3 cen;
    Vector3 axes[3];
    float extent[3];
} OBB;

typedef struct MeshSet {
    PlaneSet *meshes;
    int mesh_count;
    OBB *boxes;
    int box_count;
} MeshSet;

// entity component system 4 dummies
#define LUA_SET_SIZE 64
typedef struct ModelSet {
    Model models[LUA_SET_SIZE];
    MeshSet convexMeshBounds[LUA_SET_SIZE];
    Vector3 positions[LUA_SET_SIZE];
    float boundingSpheres[LUA_SET_SIZE];
    int count;
} ModelSet;

typedef struct AnimationSet {
    ModelAnimation *anims;
    int animsCount;
} AnimationSet;

// globals accessed by both scripts and source code
typedef struct XN_GameState {
    XN_SETTINGS *settings;
    ModelSet modelSet;
    AnimationSet animSet[LUA_SET_SIZE];
    Texture defaultTexture;
} XN_GameState;

typedef struct FloatArray {
    int size;
    float buffer[1];
} FloatArray;

// ------------------ defined in lua_util.c ---------------------------------------------

void lua_openLib(lua_State *L, const struct luaL_Reg *fns, const char *libName);

// get values from lua tables
int lua_checkField( lua_State *L, const char *key, int table_stack_index, int type);
#define lua_checkBoolField( L, key, i) \
    ({ int retval = lua_checkField(L, key, i, LUA_TBOOLEAN) == 0 ? lua_toboolean(L, -1) : -1; lua_pop(L, 1); retval; })
#define lua_checkIntField(L, key, i) \
    ({ int retval = lua_checkField(L, key, i, LUA_TNUMBER) == 0 ? luaL_checkinteger(L, -1) : -1; lua_pop(L, 1); retval; })
#define lua_checkFloatField(L, key, i) \
    ({ float retval = lua_checkField(L, key, i, LUA_TNUMBER) == 0 ? lua_tonumber(L, -1) : -1; lua_pop(L, 1); retval; })
#define lua_checkStringField(L, key, i) \
    ({ const char *retval = lua_checkField(L, key, i, LUA_TSTRING) == 0 ? luaL_checkstring(L, -1) : NULL; lua_pop(L, 1); retval; })

void lua_setFloatField(lua_State *L, const char *key, float value);
void lua_setIntField(lua_State *L, const char *key, int value);

lua_Number lua_getNumberFromTable( lua_State *L, int index, int table_stack_index );
lua_Number lua_getNumberField( lua_State *L, const char *key, int table_stack_index);
const char *lua_getStringField( lua_State *L, const char *key, int table_stack_index );

int lua_allocateFloatArray ( lua_State *L);
int lua_setFloatArray (lua_State *L);
int lua_getFloatArray (lua_State *L);
int lua_floatArraySize (lua_State *L);
int lua_floatArrayToString (lua_State *L);
// int lua_submitFloatArray( lua_State *L );
int lua_openFloatArrayLib(lua_State *L);

// error reporting
int check_lua( lua_State *L, int r );
void lua_check_script_function( lua_State *L, const char* function );
int lua_checkOOB(lua_State *L, int i, int n);

// --------------------------------------------------------------------------------------

XN_SETTINGS load_settings(lua_State *L, const char *settings_file);
XN_SETTINGS *get_settings();

XN_GameState create_XN_GameState( XN_SETTINGS *settings);
int lua_setGameState(XN_GameState *state);
XN_GameState *get_gamestate();

int lua_GetTime( lua_State *L ); // Returns elapsed time in seconds since InitWindow() 

// server
int lua_getMotionData(lua_State *L);
int lua_getConnections(lua_State *L);
int lua_popNewMessages(lua_State *L) ;
int lua_getMessageValue(lua_State *L);
int lua_messageListSize(lua_State *L);
int lua_messageListToString(lua_State *L);
int lua_setPairMode(lua_State *L);
int lua_openServerLib(lua_State *L);
int lua_get_connected(lua_State *L);

// particles
int lua_getPerlinNoise(lua_State *L);
int lua_allocateParticleSystem( lua_State *L);
int lua_setEmitterPos( lua_State *L );
int lua_getEmitterPos( lua_State *L );
int lua_updateParticles( lua_State *L );
int lua_drawParticles( lua_State *L );
int lua_resetEmitter(lua_State *L);
int lua_openParticleLib(lua_State *L);

// raylib
void lua_createColorTable(lua_State *L);
Color lua_getColor(lua_State *L, int table_stack_index);
void lua_setColor(lua_State *L, Color col, const char* name );
int lua_loadTexture(lua_State *L);
int lua_drawTexture(lua_State *L);
int lua_clearScreen(lua_State *L);
int lua_drawText(lua_State *L);
int lua_measureText(lua_State *L);
int lua_drawLine(lua_State *L);
int lua_fillTriangle( lua_State *L ); // Draw a color-filled triangle (vertex in counter-clockwise order!) 
int lua_drawRect(lua_State *L);
int lua_fillRect(lua_State *L);
int lua_fillRectRot( lua_State *L ); // Draw a color-filled rectangle with pro parameters 
int lua_drawCircle(lua_State *L);
int lua_fillCircle(lua_State *L);
int lua_fillCircleSector(lua_State *L);
int lua_BeginDrawing( lua_State *L ); // Setup canvas (framebuffer) to start drawing 
int lua_EndDrawing( lua_State *L ); // End canvas drawing and swap buffers (double buffering) 
int lua_EndTextureMode( lua_State *L ); // Ends drawing to render texture 
int lua_BeginTextureMode( lua_State *L ); // Initializes render texture for drawing 
int lua_LoadRenderTexture( lua_State *L ); // Load texture for rendering (framebuffer) 
int lua_DrawTexturePro( lua_State *L ); // resize, crop, rotate, tint, & draw texture  
int lua_LoadShader( lua_State *L ); // Load shader from files and bind default locations 
int lua_UnloadShader( lua_State *L ); // Unload shader from GPU memory (VRAM) 
int lua_BeginShaderMode( lua_State *L ); // Begin shader drawing 
int lua_EndShaderMode( lua_State *L ); // End shader drawing (use default shader) 
int lua_setShaderUniform( lua_State *L );
int lua_getShaderUniformLoc( lua_State *L);
int lua_DrawFPS( lua_State *L ); // Shows current FPS 
int lua_loadAudioStream(lua_State *L);
int lua_TakeScreenshot( lua_State *L ); // Takes a screenshot of current screen (saved a .png) 

// 3d
Vector3 lua_getVector3Field( lua_State *L, const char *key, int table_stack_idx);
Vector3 lua_getVector3( lua_State *L, int table_stack_idx);
int lua_BeginMode3D( lua_State *L ); // Initializes 3D mode with custom camera (3D) 
int lua_EndMode3D( lua_State *L ); // Ends 3D mode and returns to default 2D orthographic mode 
int lua_DrawGrid( lua_State *L );
int lua_SetCameraMode( lua_State *L );
int lua_loadModel( lua_State *L );
int lua_unloadModel( lua_State *L );
int lua_drawModel( lua_State *L );
int lua_DrawModelWires( lua_State *L ); // Draw a model wires (with texture if set) 
int lua_LoadModelMat( lua_State *L );
int lua_LoadModelAnimations( lua_State *L );
int lua_UpdateModelAnimation( lua_State *L );
int lua_getAnimFrameCount( lua_State *L );
int lua_RotateModelEuler(lua_State *L);
int lua_DrawSphereWires( lua_State *L ); // Draw sphere wires 
int lua_DrawSphere( lua_State *L ); // Draw sphere 
int lua_DrawCubeTexture( lua_State *L ); // Draw cube textured 
int lua_DrawCubeWiresV( lua_State *L ); // Draw cube wires (Vector version) 
int lua_DrawCubeV( lua_State *L ); // Draw cube (Vector version) 
int lua_loadCubeModel( lua_State *L);
int lua_getBoundingSphere( lua_State *L );
int lua_setModelPosition( lua_State *L );
int lua_drawModelBasic( lua_State *L );
int lua_transformVectorByMatrix( lua_State *L );

// physics
int lua_rotateVectorByQuaternion( lua_State *L );
int lua_QuaternionFromEuler( lua_State *L );
int lua_rotateVectorEulers( lua_State *L );
int lua_getConvexMeshBounds( lua_State *L );
int lua_Slerp( lua_State *L );
int lua_MatrixTranslate( lua_State *L );
int lua_setModelTransform( lua_State *L );
int lua_separatingAxisSphere(lua_State *L);
void mesh_load_physics_data(ModelSet *m);
int lua_getModelAABB( lua_State *L );
int lua_getConvexHull(lua_State *L);

// define lua host libraries
static const struct luaL_Reg lua_server_f[] = {
    {"pop_unsafe", lua_popNewMessages },
    {"get_connections", lua_getConnections },
    {"get_connected", lua_get_connected },
    { "get_motion", lua_getMotionData },
    { "set_pairing", lua_setPairMode},
    {0,0}
};

static const struct luaL_Reg lua_server_m[] = {
    {"get", lua_getMessageValue},
    {"size", lua_messageListSize},
    {"__tostring", lua_messageListToString},
    {0, 0}
};

static const struct luaL_Reg lua_particles_f[] = {
    {"new", lua_allocateParticleSystem },
    {0, 0}
};

static const struct luaL_Reg lua_particles_m[] = {
    // {"add", lua_addEmitter },
    {"update", lua_updateParticles },
    {"draw", lua_drawParticles },
    {"set", lua_setEmitterPos},
    {"get", lua_getEmitterPos},
    {"reset", lua_resetEmitter},
    {0, 0}
};

static const struct luaL_Reg lua_raylib[] = {
    { "get_time", lua_GetTime },
    { "begin_drawing", lua_BeginDrawing },
	{ "end_drawing", lua_EndDrawing },
	{ "end_texture_mode", lua_EndTextureMode },
    { "begin_texture_mode", lua_BeginTextureMode },
	{ "load_render_texture", lua_LoadRenderTexture },
    { "load_texture", lua_loadTexture },
    { "draw_texture", lua_drawTexture },
	{ "draw_texture_rect", lua_DrawTexturePro },
    { "clear_screen", lua_clearScreen },
    { "fill_circle_sector", lua_fillCircleSector },
    { "draw_rect", lua_drawRect },
    { "fill_rect", lua_fillRect },
    { "fill_rect_rot", lua_fillRectRot },
    { "draw_circle", lua_drawCircle },
    { "fill_circle", lua_fillCircle },
    { "draw_line", lua_drawLine },
    { "fill_triangle", lua_fillTriangle },
    { "draw_text", lua_drawText },
    { "measure_text", lua_measureText },
    { "load_shader", lua_LoadShader },
	{ "unload_shader", lua_UnloadShader },
	{ "begin_shader_mode", lua_BeginShaderMode },
	{ "end_shader_mode", lua_EndShaderMode },
	{ "get_uniform_loc", lua_getShaderUniformLoc },
	{ "set_uniform", lua_setShaderUniform },
    { "draw_fps", lua_DrawFPS },
    { "load_audiostream", lua_loadAudioStream },
    { "perlin2d", lua_getPerlinNoise},
    { "take_screenshot", lua_TakeScreenshot },
    
    { "set_camera_mode", lua_SetCameraMode },
    { "begin_3d_mode", lua_BeginMode3D },
	{ "end_3d_mode", lua_EndMode3D },
	{ "load_model", lua_loadModel },
	{ "draw_model", lua_drawModel },
	{ "draw_model_basic", lua_drawModelBasic },
    { "draw_model_wires", lua_DrawModelWires },
	{ "unload_model", lua_unloadModel },
	{ "load_model_mat", lua_LoadModelMat },
	{ "load_animations", lua_LoadModelAnimations },
	{ "update_model_animation", lua_UpdateModelAnimation },
	{ "get_animation_frame_count", lua_getAnimFrameCount },
	{ "model_rotate_euler", lua_RotateModelEuler },
	{ "draw_grid", lua_DrawGrid },
    { "draw_sphere_wires", lua_DrawSphereWires },
	{ "draw_sphere", lua_DrawSphere },
    { "draw_cube_texture", lua_DrawCubeTexture },
	{ "draw_cube_wires", lua_DrawCubeWiresV },
	{ "draw_cube", lua_DrawCubeV },
    { "load_cube_model", lua_loadCubeModel },
    { "vec_rotate_euler", lua_rotateVectorEulers },
    { "vec_rotate_quaternion", lua_rotateVectorByQuaternion },
    { "vec_transform_model_matrix", lua_transformVectorByMatrix },
    { "euler_to_quaternion", lua_QuaternionFromEuler },
    { "get_halfspace_bounds", lua_getConvexMeshBounds },
    { "quaternion_slerp", lua_Slerp },
    { "model_translate", lua_MatrixTranslate },
    { "model_transform", lua_setModelTransform },
    { "get_bounding_sphere", lua_getBoundingSphere},
    { "model_set_position", lua_setModelPosition},
    { "separating_axis_sphere", lua_separatingAxisSphere },
    { "model_get_aabb", lua_getModelAABB },
    { "model_get_convex_hull", lua_getConvexHull },
    {0,0}
};

static const struct luaL_Reg floatArrayLib_f[] = {
   {"new", lua_allocateFloatArray},
   {0,0}
};

static const struct luaL_Reg floatArrayLib_m[] = {
    {"set", lua_setFloatArray},
    {"get", lua_getFloatArray},
    {"size", lua_floatArraySize},
    {"__tostring", lua_floatArrayToString},
    {0, 0}
};

// define lua color palette
struct ColorInfo {
    Color color;        // raylib color data
    const char *name;   // name for the variable created in lua
};
static const int lua_num_colors = 27;
static const struct ColorInfo lua_color_palette[] = {
    {LIGHTGRAY,  "LIGHTGRAY"},  
    {GRAY,       "GRAY"},       
    {DARKGRAY,   "DARKGRAY"},   
    {YELLOW,     "YELLOW"},     
    {GOLD,       "GOLD"},       
    {ORANGE,     "ORANGE"},     
    {PINK,       "PINK"},       
    {RED,        "RED"},        
    {MAROON,     "MAROON"},     
    {GREEN,      "GREEN"},      
    {LIME,       "LIME"},       
    {DARKGREEN,  "DARKGREEN"},  
    {SKYBLUE,    "SKYBLUE"},    
    {BLUE,       "BLUE"},       
    {DARKBLUE,   "DARKBLUE"},   
    {PURPLE,     "PURPLE"},     
    {VIOLET,     "VIOLET"},     
    {DARKPURPLE, "DARKPURPLE"}, 
    {BEIGE,      "BEIGE"},      
    {BROWN,      "BROWN"},      
    {DARKBROWN,  "DARKBROWN"},  
    {WHITE,      "WHITE"},      
    {BLACK,      "BLACK"},      
    {BLANK,      "BLANK"},      
    {MAGENTA,    "MAGENTA"},    
    {RAYWHITE,   "RAYWHITE"},   
    { (Color) {0,0,0,0}, "TRANSPARENT" }
};

#ifdef __cplusplus
}
#endif

// --------------------------------------------------------------------------------------