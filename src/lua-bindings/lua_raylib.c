#include "lua_util.h"

// Setup canvas (framebuffer) to start drawing 
int lua_BeginDrawing( lua_State *L ) {
    BeginDrawing();
    return 0;
} 
// End canvas drawing and swap buffers (double buffering) 
int lua_EndDrawing( lua_State *L ) {
    EndDrawing();
    return 0;
}
// Ends drawing to render texture 
int lua_EndTextureMode( lua_State *L ) {
    EndTextureMode();
    return 0;
} 

// Returns elapsed time in seconds since InitWindow() 
int lua_GetTime( lua_State *L ) {
    lua_pushnumber( L, GetTime() );
    return 1;
}

// Shows current FPS 
int lua_DrawFPS( lua_State *L ) {
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    DrawFPS(x,y);
    return 0;
} 

// convert lua table to texture struct
static Texture2D lua_getTexture( lua_State *L, int table_stack_idx ) { 
    Texture2D tex = {
        (int) lua_getNumberField(L, "id", table_stack_idx),
        (int) lua_getNumberField(L, "width", table_stack_idx),
        (int) lua_getNumberField(L, "height", table_stack_idx),
        (int) lua_getNumberField(L, "mipmaps", table_stack_idx),
        (int) lua_getNumberField(L, "format", table_stack_idx)
    };
    return tex;
} 

static RenderTexture2D lua_getRenderTexture( lua_State *L, int table_stack_idx ) {
    lua_pushstring(L, "texture");
    lua_gettable(L, table_stack_idx);
    Texture2D tex = lua_getTexture( L, table_stack_idx+1);

    lua_pushstring(L, "depth");
    lua_gettable(L, table_stack_idx);
    Texture2D depth = lua_getTexture(L,table_stack_idx+1);
    
    RenderTexture2D rt = {
        lua_getNumberField(L, "id", table_stack_idx),
        tex,
        depth,
        false
    };
    return rt;
}

// push a texture table to the top of the stack
static int lua_textureToTable(lua_State *L, Texture2D t) {
    lua_newtable(L);
    lua_setIntField(L, "id", t.id);
    lua_setIntField(L, "width", t.width);
    lua_setIntField(L, "height", t.height);
    lua_setIntField(L, "format", t.format);
    lua_setIntField(L, "mipmaps", t.mipmaps);
    
    return 0;
}

// Initializes render texture for drawing 
int lua_BeginTextureMode( lua_State *L ) {
    RenderTexture2D rt = lua_getRenderTexture(L, 1);
    BeginTextureMode(rt);
    return 0;
} 

// Load texture for rendering (framebuffer) 
int lua_LoadRenderTexture( lua_State *L ) {
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    
    RenderTexture2D res = LoadRenderTexture(w,h);
    lua_newtable(L);
    lua_setIntField(L, "id", res.id);

    lua_pushstring(L, "texture");
    lua_textureToTable(L, res.texture);
    lua_settable(L, 3);

    lua_pushstring(L, "depth");
    lua_textureToTable(L, res.depth );
    lua_settable(L, 3);

    lua_pushstring(L, "depthTexture");
    lua_pushboolean(L, res.depthTexture);
    lua_settable(L, 3);

    return 1;
} 

static Rectangle lua_getRect( lua_State *L, int table_stack_idx) {
    Rectangle r = {
        lua_getNumberField(L, "x", table_stack_idx),
        lua_getNumberField(L, "y", table_stack_idx),
        lua_getNumberField(L, "w", table_stack_idx),
        lua_getNumberField(L, "h", table_stack_idx)
    };

    return r;
}

// resize, crop, rotate, tint, & draw texture
int lua_DrawTexturePro( lua_State *L ) {
    Texture2D t = lua_getTexture(L, 1);
    Rectangle src_rect = lua_getRect(L, 2);
    Rectangle dst_rect = lua_getRect(L, 3);
    float rot = luaL_checknumber(L, 4);
    Color tint = lua_getColor(L, 5);
    Vector2 origin = {0,0};

    DrawTexturePro( t, src_rect, dst_rect, origin, rot, tint );

    return 0;
} 

int lua_loadTexture(lua_State *L) {
    const char *file_name = luaL_checkstring(L,1);
    Texture2D t = LoadTexture(file_name);

    lua_newtable(L);
    lua_setIntField(L, "id", t.id);
    lua_setIntField(L, "width", t.width);
    lua_setIntField(L, "height", t.height);
    lua_setIntField(L, "format", t.format);
    lua_setIntField(L, "mipmaps", t.mipmaps);

    return 1;
} 

int lua_drawTexture(lua_State *L) {
    if ( !lua_istable(L, 1) ) {
        lua_Debug ar;
        lua_getstack(L, 1, &ar);
        lua_getinfo(L, "nSl", &ar);
        printf("[LUA] Error: In file '%s' (line: %d) invalid texture provided: '%s'.\n", ar.short_src, ar.currentline, lua_tostring(L, 1));
    } else {
        Texture2D tex = lua_getTexture(L, 1);
        int x = luaL_checkinteger(L,2);
        int y = luaL_checkinteger(L,3);
        Color tint = lua_getColor(L, 4);

        DrawTexture(tex, x, y, tint);
    }
    return 0;
}

static Shader lua_getShader( lua_State *L, int table_stack_idx ) {
    Shader shader;
    lua_pushstring(L, "id");
    lua_gettable(L, table_stack_idx);
    shader.id = luaL_checkinteger(L, -1);
    lua_pushstring(L, "locs");
    lua_gettable(L, table_stack_idx);
    shader.locs = (int*) luaL_checkinteger(L, -1);

    return shader;
}

// Load shader from files and bind default locations 
int lua_LoadShader( lua_State *L ) {
    const char *vs = luaL_checkstring(L, 1);
    const char *fs = luaL_checkstring(L, 2);
    Shader result = LoadShader( vs, fs );

    lua_newtable(L);
    lua_setIntField(L, "id", result.id);
    lua_setIntField(L, "locs", (int)result.locs);
    return 1;
} 

int lua_getShaderUniformLoc( lua_State *L) {
    Shader shader = lua_getShader(L, 1);
    const char *uniform = luaL_checkstring(L, 2);
    int loc = GetShaderLocation( shader, uniform );
    lua_pushinteger(L, loc);
    return 1;
}

int lua_setShaderUniform( lua_State *L ) {
    Shader shader = lua_getShader(L, 1);
    int uniformLoc = luaL_checkinteger(L, 2);
    int type = luaL_checkinteger(L, 4);
    switch (type) {
        case UNIFORM_FLOAT:
        {
            float n = luaL_checknumber( L, 3 );
            SetShaderValue(shader, uniformLoc, &n, type);
        }
        break;
        case UNIFORM_VEC2:
        {
            float n[2];
            lua_pushstring(L, "x");
            lua_gettable(L, 3);
            n[0] = luaL_checknumber(L, -1);
            lua_pushstring(L, "y");
            lua_gettable(L, 3);
            n[1] = luaL_checknumber(L, -1);
            SetShaderValue(shader, uniformLoc, n, type);
        }
        break;
        // case UNIFORM_VEC3:
        // break;
        // case UNIFORM_VEC4:
        // break;
        // case UNIFORM_INT:
        // break;
        // case UNIFORM_IVEC2:
        // break;
        // case UNIFORM_IVEC3:
        // break;
        // case UNIFORM_IVEC4:
        // break;
        // case UNIFORM_SAMPLER2D:
        // break;
        default:
            printf("ERROR: bad uniform type\n");
        break;
    }
    return 0;
}

// Unload shader from GPU memory (VRAM) 
int lua_UnloadShader( lua_State *L ) {
    Shader s = lua_getShader(L, 1);
    UnloadShader(s);
    return 0;
} 
// Begin custom shader drawing 
int lua_BeginShaderMode( lua_State *L ) {
    Shader s = lua_getShader(L, 1);
    BeginShaderMode(s);
    return 0;
} 
// End custom shader drawing (use default shader) 
int lua_EndShaderMode( lua_State *L ) {
    EndShaderMode();
    return 0;
} 

int lua_clearScreen(lua_State *L) {
    Color col = lua_getColor(L, 1);
    
    ClearBackground(col);
    return 0;
}

int lua_drawText(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    int font_size = luaL_checkinteger(L, 4);
    Color col = lua_getColor(L, 5);

    DrawText(text, x, y, font_size, col);

    return 0;
}

int lua_measureText(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int font_size = luaL_checkinteger(L, 2);

    lua_pushnumber(L, MeasureText(text, font_size));
    return 1;
}

int lua_drawLine(lua_State *L) {
    Vector2 start = {(float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2) };
    Vector2 end = {(float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4) };
    Color col = lua_getColor(L,5);
    float thicc = luaL_checknumber(L, 6);

    DrawLineEx( start, end, thicc, col);

    return 0;
}

// Draw a color-filled triangle (vertex in counter-clockwise order!) 
int lua_fillTriangle( lua_State *L ) {
    Vector2 v1 = { luaL_checknumber(L, 1),luaL_checknumber(L, 2)};
    Vector2 v2 = { luaL_checknumber(L, 3),luaL_checknumber(L, 4)};
    Vector2 v3 = { luaL_checknumber(L, 5),luaL_checknumber(L, 6)};
    Color col = lua_getColor(L, 7);

    DrawTriangle(v1,v2,v3, col);
    return 0;
} 

int lua_fillCircleSector(lua_State *L) {
    Vector2 center = {  luaL_checknumber(L, 1),  luaL_checknumber(L, 2)};
    float radius =  luaL_checknumber(L, 3);
    Color col = lua_getColor(L, 4);
    int startAngle =  luaL_checkinteger(L, 5);
    int endAngle =  luaL_checkinteger(L, 6); 
    int segments =  luaL_checkinteger(L, 7);
    
    DrawCircleSector( center, radius, startAngle, endAngle, segments, col);

    return 0;
}

int lua_drawRect(lua_State *L) {
    float x = (float) luaL_checknumber(L, 1), y = (float) luaL_checknumber(L, 2);
    float w = (float) luaL_checknumber(L, 3), h = (float) luaL_checknumber(L, 4);
    Color col = lua_getColor(L, 5);
    int thicc = luaL_checkinteger(L, 6);

    DrawRectangleLinesEx( (Rectangle) {x,y,w,h}, thicc, col);

    return 0;
}

int lua_fillRect(lua_State *L) {
    float x = (float) luaL_checknumber(L, 1), y = (float) luaL_checknumber(L, 2);
    float w = (float) luaL_checknumber(L, 3), h = (float) luaL_checknumber(L, 4);
    Color col = lua_getColor(L, 5);

    DrawRectangle( x, y, w, h, col);

    return 0;
}

// Draw a color-filled rectangle with pro parameters 
int lua_fillRectRot( lua_State *L ) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float w = luaL_checknumber(L, 3);
    float h = luaL_checknumber(L, 4);
    float r = luaL_checknumber(L, 5);
    Color col = lua_getColor(L, 6);
    float ox = luaL_checknumber(L, 7);
    float oy = luaL_checknumber(L, 8);
    
    Rectangle rect = {x,y,w,h};
    Vector2 origin = {ox, oy};
    DrawRectanglePro(rect, origin, r, col);
    return 0;
} 

int lua_drawCircle(lua_State *L) {
    float x = (float) luaL_checknumber(L, 1), y = (float) luaL_checknumber(L, 2);
    float r = (float) luaL_checknumber(L, 3);  
    Color col = lua_getColor(L, 4);
    float thicc = (float) luaL_checknumber(L, 5);
    int segments = (float) luaL_checkinteger(L, 6);

    DrawRing( (Vector2) {x, y}, r, r+thicc, 0, 360, segments, col );

    return 0;
}

int lua_fillCircle(lua_State *L) {
    float x = (float) luaL_checknumber(L, 1), y = (float) luaL_checknumber(L, 2);
    float r = (float) luaL_checknumber(L, 3);
    Color col = lua_getColor(L, 4);

    DrawCircle( x, y, r, col);

    return 0;
}

Color lua_getColor(lua_State *L, int table_stack_index) {
    if ( !lua_istable(L, table_stack_index) ) {
        lua_Debug ar;
        lua_getstack(L, 1, &ar);
        lua_getinfo(L, "nSl", &ar);
        printf("[LUA] Error: %s:%d: invalid color provided: '%s'. Defaulting to magenta\n", ar.short_src, ar.currentline, lua_tostring(L, table_stack_index));
        return MAGENTA;
    }
    return (Color) {
        lua_checkIntField(L, "r", table_stack_index),
        lua_checkIntField(L, "g", table_stack_index),
        lua_checkIntField(L, "b", table_stack_index),
        lua_checkIntField(L, "a", table_stack_index)
    };
}

void lua_setColorField(lua_State *L, int table_stack_index, const char *key, unsigned char value ) {
    lua_pushstring(L, key);
    lua_pushnumber(L, (float)value);
    lua_settable(L, table_stack_index-2);
}

void lua_setColor(lua_State *L, Color col, const char* name ) {
    lua_newtable(L);
    lua_setColorField(L, -1, "r", col.r);
    lua_setColorField(L, -1, "g", col.g);
    lua_setColorField(L, -1, "b", col.b);
    lua_setColorField(L, -1, "a", col.a);
    if ( name )
        lua_setglobal(L, name);
}

void lua_createColorTable(lua_State *L) {
    for ( int i = 0; i < lua_num_colors; i++ ) {
        struct ColorInfo col = lua_color_palette[i];
        lua_setColor(L, col.color, col.name);
    }
}

// --------------------------------- audio ------------------------------------

int lua_loadAudioStream(lua_State *L) {
    // const char *f_name = luaL_checkstring( L, 1);

    // Music fs = LoadMusicStream(f_name);
    // size_t nbytes = sizeof(Music);
    // Music *m = (Music*) lua_newuserdata(L, nbytes);

    // luaL_getmetatable(L, "Bob.AudioSt");
    // lua_setmetatable(L, -2);

    // m->ctxData = fs.ctxData;
    // m->ctxType = fs.ctxType;
    // m->looping = fs.looping;
    // m->sampleCount = fs.sampleCount;
    // m->stream = fs.stream;

    // return 1;
    return 0;
}