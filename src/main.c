/*******************************************************************************************
 By Austin Bobco 5/08/2020
 ********************************************************************************************/
// #include "gperftools/profiler.h" 

#define SECS_PER_UPDATE 0.01666
#define PROFILE_DURATION 30

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <fcntl.h> 
#include <sys/wait.h>
#include "lua-bindings/lua_util.h"

#include "util/bob_math.h"
#include "util/input.h"
#include "util/foreach.h"
#include "graphics/renderer.h" 
#include "graphics/particles.h"
#include "servers/bt/gamepad_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "servers/ws/ws_ring_server.c"
#include "graphics/monitor_info.c"

#ifdef __cplusplus
}
#endif


// #include "gperftools/profiler.h"

int main(int argc, const char **argv)
{
    // Initialization
    // ---------------------------------------------------------------------------------------------
    // i/o threads
    int num_gamepad_threads = 2;
    pthread_t server_thread, pair_thread, gamepad_thread[num_gamepad_threads]; 
    init_msg_q();
    for ( int i =0; i < num_gamepad_threads; i++)
        pthread_create(&gamepad_thread[i], NULL, joystick_event_thread, NULL);

    // create lua instance
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    XN_SETTINGS settings = load_settings(L);
    server_thread = ws_create_thread(settings._WEBSOCKET_DOMAIN, settings._WEBSOCKET_PORT);

    // dev console
    set_conio_terminal_mode();  // this is sum bullshit to nonblocking keypresses over ssh
    // char terminal_input_buffer[512] = "";
    TerminalInfo terminal = create_TerminalInfo(L);

    // this gets the true resolution for any monitor, but raylib is somehow stopping it from using that resolution.
    // raylib is forcing the window to be the size chosen in raspi-config (720x480), while the true monitor
    // resolution is adjusted by the OS to fit the display hardware's aspect ratio (e.g. 612x418)

    // Vector2 display = get_display_dimensions();
    
    SetTraceLogLevel(settings._LOG_LEVEL);
    InitWindow(settings._SCREEN_WIDTH, settings._SCREEN_HEIGHT, "XNgine");
    // InitAudioDevice();
    DrawGrid(10, 1);

    // trail/exposion effectsc
    create_particle_texture(PARTICLE_TEXTURE_SIZE, SHAPE_CIRCLE, 0);
    create_particle_texture(PARTICLE_TEXTURE_SIZE, SHAPE_RECT, 1);
    create_particle_texture(PARTICLE_TEXTURE_SIZE, SHAPE_TRI, 2);

    // give game state reference to lua
    XN_GameState game_state = create_XN_GameState(&settings);
    lua_setGameState(&game_state);

    // start bluetooth pairing 
    // pthread_create(&pair_thread, NULL, sync_loop, &game_state);

    // load game script
    if ( argc > 1 )
        check_lua(L, luaL_dofile(L, argv[1]));
    else 
        check_lua(L, luaL_dofile(L, "../lua/main.lua"));
    //--------------------------------------------------------------------------------------

    // ProfilerStart("XN-Tron-RPI-pass.prof"); // start recording performance
    
    // Main game loop
    // --------------------------------------------------------------------------------------
    
    SetTargetFPS(60);               
    double previous_frame_time = GetTime();
    double lag = 0;

    while ( !WindowShouldClose() ) 
    {   
        handle_keyboard_input(&terminal);

        // get elapsed time since last frame, add to lag counter
        double current_time = GetTime();
        double elapsed_time = current_time - previous_frame_time;
        previous_frame_time = current_time; 
        lag += elapsed_time;
        
        while ( lag >= SECS_PER_UPDATE ) {
            // frame-rate independent stuff goes in here
            
            lua_check_script_function( L, "_fixedUpdate");
            lag -= SECS_PER_UPDATE; 
        } 

        draw_scene(&terminal); // do lua draw commands, draw terminal if open
    }
    //--------------------------------------------------------------------------------------
    
    // Cleanup
    // --------------------------------------------------------------------------------------
    // ProfilerStop();                  // stop recording performance
    // CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
    
    for (int i =0; i< num_gamepad_threads; i++ )  {
        pthread_cancel(gamepad_thread[i]);
    } 
    interrupted = true;
    pthread_join(server_thread, NULL); 
    // pthread_cancel(pair_thread);
    // pthread_join(pair_thread, NULL); 
    lua_close(L);                       
    reset_terminal_mode();
    CloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}