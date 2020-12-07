#include "renderer.h"
#include <string.h>
#include <math.h>
#include "../util/bob_math.h"
#include "rlgl.h"

// draw player 1's gamepad values to the screen for debugging
void draw_gamepad_state(GamepadNumber gp) {
    int offset = 30;
    // draw axis count
    DrawText(FormatText("DETECTED AXES: %i", GetGamepadAxisCount(gp)), offset, 50, 10, MAROON);
    // draw axis input values
    for (int i = 0; i < GetGamepadAxisCount(gp); i++) {
        DrawText(FormatText("AXIS %i: %.02f", i, GetGamepadAxisMovement(gp, i)), offset+10, 70 + 20*i, 10, DARKGRAY);
    }
    // draw button state
    if (GetGamepadButtonPressed() != -1) 
        DrawText(FormatText("DETECTED BUTTON: %i", GetGamepadButtonPressed()), offset, 430, 10, RED);
    else 
        DrawText("DETECTED BUTTON: NONE", offset, 430, 10, GRAY);
}

void draw_particle_texture(ParticleInfo particle, ParticleTexture pt, Color color) {
    DrawTextureEx(
        pt._texture.texture,
        particle.position,
        0,
        particle.scale, 
        color
    );
}

// make an edge detection post processing filter 
PostProcessingFilter* create_sobel_filter(int screen_width, int screen_height, const char *fs_file) {
    PostProcessingFilter* filter = (PostProcessingFilter *) malloc( sizeof( PostProcessingFilter ) );
    filter->render_target = LoadRenderTexture(screen_width, screen_height);

    // NOTE: the path for the shader file needs to be relative to the directory that the game was run from
    // filter->shader = LoadShader(0, FormatText("resources/shaders/glsl%i/sobel.fs", GLSL_VERSION));
    filter->shader = LoadShader(0, fs_file);

    return filter;
}

void fill_tex(RenderTexture2D tex, Color color) {
    BeginDrawing();
    BeginTextureMode(tex);
    ClearBackground(color);
    EndTextureMode();
    EndDrawing();
}

void draw_particles(EmitterInfo* emitter) {
    for ( int j = 0; j < emitter->num_particles; j++ ) {
        draw_particle_texture(emitter->particles[j], emitter->texture, emitter->color);
    }
}

void draw_model_wires(Model* model, Vector3 position, Color color) {
        rlTranslatef(position.x, position.y, position.z);
        //rlRotatef(45, 0, 1, 0);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);
            for ( int i = 0; i < model->meshCount; i++ ) {
                for ( int j = 0; j < model->meshes[i].triangleCount; j++ ) {
                   unsigned int tri_idx = model->meshes[i].indices[j*3]*3;
                    Vector3 v1 = get_vert(model->meshes[i].vertices, tri_idx);
                    Vector3 v2 = get_vert(model->meshes[i].vertices, tri_idx+1);
                    Vector3 v3 = get_vert(model->meshes[i].vertices, tri_idx+2);

                    rlVertex3f(v1.x, v1.y, v1.z);
                    rlVertex3f(v2.x, v2.y, v2.z);

                    rlVertex3f(v1.x, v1.y, v1.z);
                    rlVertex3f(v3.x, v3.y, v3.z);

                    rlVertex3f(v2.x, v2.y, v2.z);
                    rlVertex3f(v3.x, v3.y, v3.z);
                }
            }           
        rlEnd();
    rlPopMatrix();
}

void draw_scene(TerminalInfo* terminal) {
    XN_GameState *g = get_gamestate();
    BeginDrawing();
    ClearBackground((Color) {30, 30, 30, 255});

    lua_check_script_function(terminal->L, "_draw");
    // draw lua terminal
    if ( terminal->isOpen ) {
        DrawRectangle( 0, 0, g->settings->_SCREEN_WIDTH, 120, (Color) {30, 30, 30, 150} );

        char disp_txt[514] = "> ";
        strcat(disp_txt, terminal->input);
        DrawText((const char*)disp_txt, 30, 80, 20, MAROON);

        if ( fmod(GetTime(), 1.0) > 0.5 ) {
            // get cursor screen pos
            char before_cursor[514];
            strcpy(before_cursor, disp_txt);
            for ( int i = 2 + terminal->cursorPos; i < strlen(before_cursor); i++ ) {
                before_cursor[i] = '\0';
            }  

            // draw cursor
            DrawRectangle( 32 + MeasureText(before_cursor, 20), 80 , 10,20, MAROON);
        }        
    }

    EndDrawing();
}


void draw_scene_min(TerminalInfo* terminal) {
    XN_GameState *g = get_gamestate();
    // BeginDrawing();
    // ClearBackground((Color) {30, 30, 30, 255});

    lua_check_script_function(terminal->L, "_draw");
    // draw lua terminal
    if ( terminal->isOpen ) {
        DrawRectangle( 0, 0, g->settings->_SCREEN_WIDTH, 120, (Color) {30, 30, 30, 150} );

        char disp_txt[514] = "> ";
        strcat(disp_txt, terminal->input);
        DrawText((const char*)disp_txt, 30, 80, 20, MAROON);

        if ( fmod(GetTime(), 1.0) > 0.5 ) {
            // get cursor screen pos
            char before_cursor[514];
            strcpy(before_cursor, disp_txt);
            for ( int i = 2 + terminal->cursorPos; i < strlen(before_cursor); i++ ) {
                before_cursor[i] = '\0';
            }  

            // draw cursor
            DrawRectangle( 32 + MeasureText(before_cursor, 20), 80 , 10,20, MAROON);
        }        
    }

    // EndDrawing();
}