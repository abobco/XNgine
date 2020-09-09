#include "lua_util.h"
#include "raymath.h"
#include "raylib.h"

#include "../graphics/renderer.h"

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

// clear active framebuffer to a color
int lua_clearScreen(lua_State *L) {
    Color col = lua_getColor(L, 1);
    
    ClearBackground(col);
    return 0;
}

// --------------------------------- 2D -------------------------------------------------

static Rectangle lua_getRect( lua_State *L, int table_stack_idx) {
    Rectangle r = {
        lua_getNumberField(L, "x", table_stack_idx),
        lua_getNumberField(L, "y", table_stack_idx),
        lua_getNumberField(L, "w", table_stack_idx),
        lua_getNumberField(L, "h", table_stack_idx)
    };

    return r;
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

// load image texture from file
int lua_loadTexture(lua_State *L) {
    const char *file_name = luaL_checkstring(L,1);
    lua_textureToTable( L, LoadTexture(file_name) );
    return 1;
} 

// convert lua table to texture struct
static Texture2D lua_getTexture( lua_State *L, int table_stack_idx ) { 
    Texture2D tex = {
        lua_getNumberField(L, "id", table_stack_idx),
        lua_getNumberField(L, "width", table_stack_idx),
        lua_getNumberField(L, "height", table_stack_idx),
        lua_getNumberField(L, "mipmaps", table_stack_idx),
        lua_getNumberField(L, "format", table_stack_idx)
    };
    return tex;
} 

// load a texture that can be used as a render target
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

// Initializes render texture for drawing 
int lua_BeginTextureMode( lua_State *L ) {
    RenderTexture2D rt = lua_getRenderTexture(L, 1);
    BeginTextureMode(rt);
    return 0;
} 

// Ends drawing to render texture 
int lua_EndTextureMode( lua_State *L ) {
    EndTextureMode();
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

// move, tint, draw an unscaled, unrotated texture
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
        // TODO:
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

// --------------------------------- 3D -------------------------------------------------

Vector3 lua_getVector3( lua_State *L, int table_stack_idx) {
    Vector3 pos;
    pos.x = lua_checkFloatField(L, "x", table_stack_idx);
    pos.y = lua_checkFloatField(L, "y", table_stack_idx);
    pos.z = lua_checkFloatField(L, "z", table_stack_idx);
    // printf("%f, %f, %f \n", pos.x, pos.y, pos.z);
    return pos;
}

Vector3 lua_getVector3Field( lua_State *L, const char *key, int table_stack_idx) {
    lua_pushstring(L, key);
    lua_gettable(L, table_stack_idx);
    return lua_getVector3(L, -1);
}

static Camera lua_getCamera( lua_State *L, int table_stack_idx ) {
    Camera cam;
    cam.position = lua_getVector3Field(L, "position", 1);
    cam.target = lua_getVector3Field(L, "target", 1);
    cam.up = lua_getVector3Field(L, "up", 1);
    cam.fovy = lua_checkFloatField(L, "fovy", 1);
    cam.type = lua_checkIntField(L, "type", 1);
    // printf("%f\n", cam.fovy);

    return cam;
}

int lua_testCam(lua_State *L) {
    lua_getCamera(L, 1);
    return 0;
}

int lua_SetCameraMode( lua_State *L ) {
    Camera cam = lua_getCamera(L, 1);
    int mode = luaL_checkinteger(L, 2);

    SetCameraMode(cam, mode);
    return 0;
}

// Initializes 3D mode with custom camera (3D) 
int lua_BeginMode3D( lua_State *L ) {
    Camera cam = lua_getCamera(L, 1);
    BeginMode3D(cam);
    return 0;
} 
// Ends 3D mode and returns to default 2D orthographic mode 
int lua_EndMode3D( lua_State *L ) {
    
    EndMode3D();
    return 0;
} 

int lua_DrawGrid( lua_State *L ) {
    DrawGrid( luaL_checkinteger(L, 1), luaL_checknumber(L, 2));
    return 0;
}

int lua_loadModel( lua_State *L ) {
    const char *mfile = luaL_checkstring(L, 1);
    ModelSet *m =  &get_gamestate()->modelSet;
    m->models[m->count] = LoadModel(mfile);
    m->models[m->count].materials[0].maps[MAP_DIFFUSE].texture = get_gamestate()->defaultTexture;
    int mesh_count = m->models[m->count].meshCount;
    m->convexMeshBounds[m->count].meshes = malloc( sizeof(PlaneSet) * mesh_count );
    m->convexMeshBounds[m->count].count = mesh_count;

    printf("Model '%s' loaded:\t %d mesh%s:\n", mfile, mesh_count, mesh_count > 1 ? "es":"" );

    int convex_polys = 0;
    for ( int ii = 0; ii < mesh_count; ii++ ) {
        Mesh *mesh = &m->models[m->count].meshes[ii];

        // O(n^2) polyhedron convexity test
        // TODO: replace w/ O(n) centroid method
        bool is_convex = true;
        for ( int i=0; i < mesh->triangleCount && is_convex; i++ ) {
            unsigned int sup_idx = mesh->indices[i*3]*3;
            Vector3 supporting_pt = get_vert(mesh->vertices, sup_idx);        
            Vector3 normal = get_vert(mesh->normals, sup_idx);  

            for ( int j = 0; j < mesh->vertexCount && is_convex; j+=3 ) {
                Vector3 vert = get_vert(mesh->vertices, j);  
                if ( halfspace_point(supporting_pt, normal, vert) > EPSILON )
                    is_convex = false;
            }  
        }

        // get local halfspace bounds for separating axis tests
        if (is_convex) {
            convex_polys++;
            struct PlaneNode { 
                Plane plane; 
                struct PlaneNode *next; 
            };
            struct PlaneNode *bounds_list = NULL;
            struct PlaneNode *p_iter = bounds_list;
            int plane_count = 0;

            // get a temp linked list of unique planes from the triangle mesh
            for ( int i=0; i < mesh->triangleCount; i++ ) {
                unsigned int sup_idx = mesh->indices[i*3]*3;
                Plane new_plane = {get_vert(mesh->vertices, sup_idx),  get_vert(mesh->normals, sup_idx)};
                bool unique = true;
                
                // reject if new_plane's position lies on any previous plane & shares a normal w/ that plane 
                p_iter = bounds_list;
                while ( p_iter != NULL && unique ) {
                    if ( fabsf( halfspace_point( p_iter->plane.point, p_iter->plane.normal, new_plane.point ) ) < EPSILON 
                    && fabsf( Vector3DotProduct( p_iter->plane.normal, new_plane.normal ) - 1.0f ) < EPSILON ) {
                        unique = false;
                    }
                    p_iter = p_iter->next;
                }
                if ( !unique ) continue;   

                // insert new plane
                struct PlaneNode *new_node = (struct PlaneNode*) malloc(sizeof(struct PlaneNode));
                p_iter = bounds_list;
                new_node->plane =  new_plane;
                new_node->next = NULL;
                plane_count++;
                if ( bounds_list == NULL ) {
                    bounds_list = new_node;
                    continue;
                }
                while ( p_iter->next != NULL ) {  
                    p_iter = p_iter->next;
                }   
                p_iter->next = new_node;
            }

            // copy to global array of physics data
            PlaneSet *bounds = &m->convexMeshBounds[m->count].meshes[ii];
            bounds->planes = malloc( sizeof(Plane) * plane_count );
            bounds->count = 0;
            p_iter = bounds_list;
            while( p_iter != NULL ) {
                bounds->planes[bounds->count++] = p_iter->plane;
                p_iter = p_iter->next;
            }

            // free temp list
            while ( bounds_list != NULL) {
                p_iter=bounds_list;
                bounds_list = bounds_list->next;
                free(p_iter);
            }
            // for ( int i = 0; i < bounds->count; i++ ) {
            //     print(i);
            //     print_vec(bounds->planes[i].point);
            //     print_vec(bounds->planes[i].normal);
            // }
        }
        
        char mesh_type[32] = "";
        if (is_convex) strcpy(mesh_type, "convex");
        else           strcpy(mesh_type, "\033[1;31mconcave\033[0m");

        printf("\tMesh %d: %d triangles, %d verts, %s", ii, mesh->triangleCount, mesh->vertexCount, mesh_type);
        if (is_convex) printf(" bounded by %d planes", m->convexMeshBounds[m->count].meshes[ii].count);
        printf("\n");
    }
    m->convexMeshBounds[m->count].count = convex_polys;
    lua_pushinteger(L, m->count++);

    return 1;
}

int lua_unloadModel( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);

    XN_GameState *g = get_gamestate();
    UnloadModel(g->modelSet.models[id]);

    if ( g->animSet[id].anims != NULL ) {
        for (int i = 0; i < g->animSet[id].animsCount; i++) 
            UnloadModelAnimation(g->animSet[id].anims[i]);
        RL_FREE(g->animSet[id].anims);
    }

    return 0;
}

int lua_LoadModelMat( lua_State *L ) {
    int model_id = luaL_checkinteger(L, 1);
    const char *texfile = luaL_checkstring(L, 2);
    int mat_id = luaL_checkinteger(L, 3);

    SetMaterialTexture( &get_gamestate()->modelSet.models[model_id].materials[mat_id], 
                        MAP_DIFFUSE, LoadTexture(texfile) );

    return 0;
}

int lua_RotateModelEuler(lua_State *L) {
    get_gamestate()->modelSet.models[luaL_checkinteger(L, 1)].transform = 
        MatrixRotateXYZ( (Vector3) { luaL_checknumber(L,2),
                                     luaL_checknumber(L,3),
                                     luaL_checknumber(L,4) });
    return 0;
}

static void draw_model_generic( lua_State *L, void (*func) (Model, Vector3, Vector3, float, Vector3, Color) ) {
    int id = luaL_checkinteger(L, 1);
    (*func) (
        get_gamestate()->modelSet.models[id], 
        lua_getVector3(L, 2),
        lua_getVector3(L, 3),
        luaL_checknumber(L, 4),
        lua_getVector3(L, 5),
        lua_getColor(L, 6)
    );

    // for ( int i =0; i < get_gamestate()->modelSet.models[id].meshes[0].vertexCount*3; i+=3 ) {
    //     printf("%f\n", get_gamestate()->modelSet.models[id].meshes[0].vertices[i+1]);
    // } 
}


// Draw a model with texture if set
int lua_drawModel( lua_State *L ) {
    draw_model_generic(L, &DrawModelEx);
    return 0;
}

// Draw a model wires (with texture if set) 
int lua_DrawModelWires( lua_State *L ) {
    // draw_model_generic(L, &DrawModelWiresEx);
    int id = luaL_checkinteger(L, 1);
    draw_model_wires(&get_gamestate()->modelSet.models[id], lua_getVector3(L, 2), lua_getColor(L, 3));
    return 0;
} 

int lua_LoadModelAnimations( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    const char *mfile = luaL_checkstring(L, 2);

    get_gamestate()->animSet[id].anims = LoadModelAnimations(mfile, &get_gamestate()->animSet[id].animsCount );

    return 0;
}

int lua_UpdateModelAnimation( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    int anim_id = luaL_checkinteger(L, 2);
    int animFrame = luaL_checkinteger(L, 3);

    UpdateModelAnimation( get_gamestate()->modelSet.models[id], 
                          get_gamestate()->animSet[id].anims[anim_id], 
                          animFrame);

    return 0;
}

int lua_getAnimFrameCount( lua_State *L ) {
    int id = luaL_checkinteger(L, 1);
    int anim_id = luaL_checkinteger(L, 2);

    lua_pushinteger(L, get_gamestate()->animSet[id].anims[anim_id].frameCount);

    return 1;
}

// Draw sphere wires 
int lua_DrawSphereWires( lua_State *L ) {
    Vector3 cen = lua_getVector3(L, 1);
    float r = luaL_checknumber(L, 2);
    int rings = luaL_checkinteger(L, 3);
    int slices = luaL_checkinteger(L, 4);
    Color col = lua_getColor(L, 5);

    DrawSphereWires(cen, r, rings, slices, col);
    return 0;
} 
// Draw sphere 
int lua_DrawSphere( lua_State *L ) {
    Vector3 cen = lua_getVector3(L, 1);
    float r = luaL_checknumber(L, 2);
    Color col = lua_getColor(L, 3);

    DrawSphere(cen, r, col);
    return 0;
} 

static void draw_cube_generic( lua_State *L, void (*func) (Vector3, Vector3, Color) ) {
    Vector3 pos = lua_getVector3(L, 1);
    Vector3 size = lua_getVector3(L, 2);
    Color col = lua_getColor(L, 3);
    (*func)(pos, size, col);
}

// Draw cube wires (Vector version) 
int lua_DrawCubeWiresV( lua_State *L ) {
    draw_cube_generic(L, DrawCubeWiresV );
    return 0;
} 
// Draw cube (Vector version) 
int lua_DrawCubeV( lua_State *L ) {
    draw_cube_generic(L, DrawCubeV );
    return 0;
} 

// Draw cube textured 
int lua_DrawCubeTexture( lua_State *L ) {
    Texture2D tex = lua_getTexture(L, 1);
    Vector3 pos = lua_getVector3(L, 2);
    Vector3 size = lua_getVector3(L, 3);
    Color col = lua_getColor(L, 4);
    DrawCubeTexture(tex, pos, size.x, size.y, size.z, col);
    return 0;
} 

int lua_loadCubeModel( lua_State *L) {
    Vector3 scale = lua_getVector3(L, 1);
    Mesh cube_mesh = GenMeshCube( scale.x, scale.y, scale.z);

    ModelSet* m = &get_gamestate()->modelSet;
    m->models[m->count] = LoadModelFromMesh(cube_mesh);

    lua_pushinteger(L, m->count++);
    return 1;
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