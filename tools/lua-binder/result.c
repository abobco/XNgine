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
// Initializes render texture for drawing 
int lua_BeginTextureMode( lua_State *L ) {
    BeginTextureMode();
    return 0;
} 
// Load texture for rendering (framebuffer) 
int lua_LoadRenderTexture( lua_State *L ) {
    LoadRenderTexture();
    return 0;
} 
// Draw a part of a texture defined by a rectangle with 'pro' parameters 
int lua_DrawTexturePro( lua_State *L ) {
    DrawTexturePro();
    return 0;
} 
// Returns elapsed time in seconds since InitWindow() 
int lua_GetTime( lua_State *L ) {
    GetTime();
    return 0;
} 
// Load shader from files and bind default locations 
int lua_LoadShader( lua_State *L ) {
    LoadShader();
    return 0;
} 
// Unload shader from GPU memory (VRAM) 
int lua_UnloadShader( lua_State *L ) {
    UnloadShader();
    return 0;
} 
// Begin custom shader drawing 
int lua_BeginShaderMode( lua_State *L ) {
    BeginShaderMode();
    return 0;
} 
// End custom shader drawing (use default shader) 
int lua_EndShaderMode( lua_State *L ) {
    EndShaderMode();
    return 0;
} 
// Shows current FPS 
int lua_DrawFPS( lua_State *L ) {
    DrawFPS();
    return 0;
} 
// Draw a color-filled rectangle with pro parameters 
int lua_DrawRectanglePro( lua_State *L ) {
    DrawRectanglePro();
    return 0;
} 
// Draw a color-filled triangle (vertex in counter-clockwise order!) 
int lua_DrawTriangle( lua_State *L ) {
    DrawTriangle();
    return 0;
} 
// Initializes 3D mode with custom camera (3D) 
int lua_BeginMode3D( lua_State *L ) {
    BeginMode3D();
    return 0;
} 
// Ends 3D mode and returns to default 2D orthographic mode 
int lua_EndMode3D( lua_State *L ) {
    EndMode3D();
    return 0;
} 