void BeginDrawing(void);                                                // Setup canvas (framebuffer) to start drawing
void EndDrawing(void);                                                  // End canvas drawing and swap buffers (double buffering)
void EndTextureMode(void);                                              // Ends drawing to render texture
void BeginTextureMode(RenderTexture2D target);                          // Initializes render texture for drawing
RenderTexture2D LoadRenderTexture(int width, int height);               // Load texture for rendering (framebuffer)
void DrawTexturePro(Texture2D texture, Rectangle sourceRec, Rectangle destRec, Vector2 origin, float rotation, Color tint);       // Draw a part of a texture defined by a rectangle with 'pro' parameters
double GetTime(void);                                                   // Returns elapsed time in seconds since InitWindow()
Shader LoadShader(const char *vsFileName, const char *fsFileName);                                  // Load shader from files and bind default locations
void UnloadShader(Shader shader);                                                                   // Unload shader from GPU memory (VRAM)
void BeginShaderMode(Shader shader);                                                                // Begin custom shader drawing
void EndShaderMode(void);                                                                           // End custom shader drawing (use default shader)
void DrawFPS(int posX, int posY);                                                                 // Shows current FPS
void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);                  // Draw a color-filled rectangle with pro parameters
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);                                 // Draw a color-filled triangle (vertex in counter-clockwise order!)
void BeginMode3D(Camera3D camera);                                      // Initializes 3D mode with custom camera (3D)
void EndMode3D(void);                                                   // Ends 3D mode and returns to default 2D orthographic mode
void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color);         // Draw sphere wires
void DrawSphere(Vector3 centerPos, float radius, Color color);                                     // Draw sphere
void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color); // Draw cube textured
void DrawCubeWiresV(Vector3 position, Vector3 size, Color color);                                  // Draw cube wires (Vector version)
void DrawCubeV(Vector3 position, Vector3 size, Color color);                                       // Draw cube (Vector version)
void DrawModelWires(Model model, Vector3 position, float scale, Color tint);                       // Draw a model wires (with texture if set)