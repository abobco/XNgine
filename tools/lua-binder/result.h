int lua_BeginDrawing( lua_State *L ); // Setup canvas (framebuffer) to start drawing 
int lua_EndDrawing( lua_State *L ); // End canvas drawing and swap buffers (double buffering) 
int lua_EndTextureMode( lua_State *L ); // Ends drawing to render texture 
int lua_BeginTextureMode( lua_State *L ); // Initializes render texture for drawing 
int lua_LoadRenderTexture( lua_State *L ); // Load texture for rendering (framebuffer) 
int lua_DrawTexturePro( lua_State *L ); // Draw a part of a texture defined by a rectangle with 'pro' parameters 
int lua_GetTime( lua_State *L ); // Returns elapsed time in seconds since InitWindow() 
int lua_LoadShader( lua_State *L ); // Load shader from files and bind default locations 
int lua_UnloadShader( lua_State *L ); // Unload shader from GPU memory (VRAM) 
int lua_BeginShaderMode( lua_State *L ); // Begin custom shader drawing 
int lua_EndShaderMode( lua_State *L ); // End custom shader drawing (use default shader) 
int lua_DrawFPS( lua_State *L ); // Shows current FPS 
int lua_DrawRectanglePro( lua_State *L ); // Draw a color-filled rectangle with pro parameters 
int lua_DrawTriangle( lua_State *L ); // Draw a color-filled triangle (vertex in counter-clockwise order!) 
int lua_BeginMode3D( lua_State *L ); // Initializes 3D mode with custom camera (3D) 
int lua_EndMode3D( lua_State *L ); // Ends 3D mode and returns to default 2D orthographic mode 
int lua_DrawSphereWires( lua_State *L ); // Draw sphere wires 
int lua_DrawSphere( lua_State *L ); // Draw sphere 
int lua_DrawCubeTexture( lua_State *L ); // Draw cube textured 
int lua_DrawCubeWiresV( lua_State *L ); // Draw cube wires (Vector version) 
int lua_DrawCubeV( lua_State *L ); // Draw cube (Vector version) 
int lua_DrawModelWires( lua_State *L ); // Draw a model wires (with texture if set) 

static const struct luaL_Reg bindings[] = {
	{ "begin_drawing", lua_BeginDrawing },
	{ "end_drawing", lua_EndDrawing },
	{ "end_texture_mode", lua_EndTextureMode },
	{ "begin_texture_mode", lua_BeginTextureMode },
	{ "load_render_texture", lua_LoadRenderTexture },
	{ "draw_texture_pro", lua_DrawTexturePro },
	{ "get_time", lua_GetTime },
	{ "load_shader", lua_LoadShader },
	{ "unload_shader", lua_UnloadShader },
	{ "begin_shader_mode", lua_BeginShaderMode },
	{ "end_shader_mode", lua_EndShaderMode },
	{ "draw_f_p_s", lua_DrawFPS },
	{ "draw_rectangle_pro", lua_DrawRectanglePro },
	{ "draw_triangle", lua_DrawTriangle },
	{ "begin_mode3_d", lua_BeginMode3D },
	{ "end_mode3_d", lua_EndMode3D },
	{ "draw_sphere_wires", lua_DrawSphereWires },
	{ "draw_sphere", lua_DrawSphere },
	{ "draw_cube_texture", lua_DrawCubeTexture },
	{ "draw_cube_wires_v", lua_DrawCubeWiresV },
	{ "draw_cube_v", lua_DrawCubeV },
	{ "draw_model_wires", lua_DrawModelWires },
	{0,0}	// terminator
};