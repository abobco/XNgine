/*******************************************************************************************
 By Austin Bobco 5/08/2020
 ********************************************************************************************/

#define SECS_PER_UPDATE 0.01666
// #define PROFILING 0
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

#include "XNlib/xn_ik.hpp"
#include <vector>

#ifdef PROFILING 
#include "gperftools/profiler.h" 
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "servers/ws/ws_ring_server.c"
#include "graphics/monitor_info.c"

#ifdef __cplusplus
}
#endif

float arm_len = 104; // in milimeters
int mode = 0;
int max_mode = 4;

// camera
float camera_ang = 0; 
float camera_rad = 3;
float camera_height = 2;

void stop(int signum) { run = 0; }

Vector3 xn_to_rl(xn::vec3 vec) {
    return (Vector3) {vec.x, vec.y, vec.z};
}

int main(int argc, const char **argv)
{
    // Initialization
    // ---------------------------------------------------------------------------------------------
    // i/o threads
    const int num_gamepad_threads = 2;
    pthread_t server_thread, pair_thread, gamepad_thread[num_gamepad_threads]; 
    init_msg_q();
    for ( int i =0; i < num_gamepad_threads; i++)
        pthread_create(&gamepad_thread[i], NULL, joystick_event_thread, NULL);

    char *settings_file;
    if ( argc > 2 ) {
        settings_file =  (char *) malloc(strlen(argv[2]));
        strcpy(settings_file, argv[2]);
    } else {
        // load default settings file
        char relpath[] = "/lua/settings.lua";
        char buf[512]; 

        // get length of filepath to the main XNgine folder
        readlink("/proc/self/exe", buf, 512);
        char *prefix = strtok(buf, "/");
        int pos = 0;
        while( prefix != NULL ) {
            if ( strcmp( prefix, "build") == 0 )
                break;
            pos += strlen(prefix) + 1;  
            prefix = strtok(NULL, "/");
        }

        // copy path to main XNgine folder
        readlink("/proc/self/exe", buf, 512);
        settings_file = (char *) malloc(pos + strlen(relpath));
        strcpy(settings_file, buf);
        settings_file[pos] = 0;
        strcat(settings_file, relpath); // append relative path to settings
    } 

    // create lua instance
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    XN_SETTINGS settings = load_settings(L, settings_file);
    server_thread = ws_create_thread(settings._WEBSOCKET_DOMAIN, settings._WEBSOCKET_PORT);

    // dev console
    set_conio_terminal_mode();  // this is sum bullshit to get nonblocking keypresses over ssh
    TerminalInfo terminal = create_TerminalInfo(L);

    // this gets the true resolution for any monitor, but raylib is somehow stopping it from using that resolution.
    // raylib is forcing the window to be the size chosen in raspi-config (720x480), while the true monitor
    // resolution is adjusted by the OS to fit the display hardware's aspect ratio (e.g. 612x418)

    // Vector2 display = get_display_dimensions();
    
    SetTraceLogLevel(settings._LOG_LEVEL);
    InitWindow(settings._SCREEN_WIDTH, settings._SCREEN_HEIGHT, "XNgine");
    // InitAudioDevice();
    DrawGrid(10, 1);

    // trail/exposion effects
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
        check_lua(L, luaL_dofile(L, "../lua/yamsim.lua"));
    //--------------------------------------------------------------------------------------

    // virtual robot arm setup
    // ---------------------------------------------------------------------------------------------
    #ifndef PIO_VIRTUAL  
    if (gpioInitialise() < 0) return -1;
    gpioSetSignalFunc(SIGINT, stop);
    #endif
    xn::vec3 pole = {0, 0, 1};
    xn::Transform bonechain[] = {
        xn::Transform({0, 0, 0}),
        xn::Transform({0, 1, 0}),
        xn::Transform({0, 1 + 7.5/18, 0}),
        xn::Transform({0, 2 + 7.5/18, 0})
    };
    float start_bone_lengths[] = {
        xn::vec3::dist(bonechain[0].position, bonechain[1].position),
        xn::vec3::dist(bonechain[1].position, bonechain[2].position)
    };

    xn::ik::IkChain arm(4, bonechain, pole);
    
    // phys ik test code
    std::vector<xn::pio::ServoAngular> servos[] = {
        std::vector<xn::pio::ServoAngular>(),
        std::vector<xn::pio::ServoAngular>(),
        std::vector<xn::pio::ServoAngular>(),
        std::vector<xn::pio::ServoAngular>()
    };
    servos[0].push_back(xn::pio::ServoAngular(12, 500, 2500));
    servos[0].push_back(xn::pio::ServoAngular(13, 500, 2500));
    servos[1].push_back(xn::pio::ServoAngular(26, 575, 2460));
    servos[2].push_back(xn::pio::ServoAngular(16, 575, 2460));

    printf("%f %f\n", servos[0][1].min_angle, servos[0][1].max_angle);

    std::vector<xn::vec3> axes[] = {
        std::vector<xn::vec3>(),
        std::vector<xn::vec3>(),
        std::vector<xn::vec3>(),
        std::vector<xn::vec3>()
    };

    axes[0].push_back({0,1,0});
    axes[0].push_back({1,0,0});
    axes[1].push_back({1,0,0});
    axes[2].push_back({-1,0,0});

    xn::ik::ServoChain phys_arm(arm, servos, axes, &run, arm_len);
    phys_arm.reset();
    // time_sleep(0.5);
    start_bone_lengths[0] = xn::vec3::dist(phys_arm.positions[0], phys_arm.positions[1]);
    start_bone_lengths[1] = xn::vec3::dist(phys_arm.positions[1], phys_arm.positions[2]);
    // ---------------------------------------------------------------------------------------------

#ifdef PROFILING
    ProfilerStart("XNgine-pass.prof"); // start recording performance
#endif
    // Main game loop
    // --------------------------------------------------------------------------------------
    
    Camera cam = { 
        (Vector3) {cosf(camera_ang)*camera_rad, camera_height, sinf(camera_ang)*camera_rad},
        (Vector3) {0,0,0},
        (Vector3) {0,1,0},
        60, CAMERA_PERSPECTIVE
    };

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
        xn::vec3 target = {0,0,0};
        
        while ( lag >= SECS_PER_UPDATE ) {
            // frame-rate independent stuff goes in here
            
            lua_check_script_function( L, "_fixedUpdate");
            lag -= SECS_PER_UPDATE; 

            static float i = 0;
            static float iter = 0.01;

            if (i+iter > 1|| i+iter < 0 )
            iter*=-1;
            
            i+= iter;

            static float ang = 0;
            ang = ang+0.01;

            xn::vec3 pole;

            switch(mode){
            case 0:
            target = {-1+i*2 , 1, 1};
            pole = {-2+i*4 , 1, 2};
            break;
            case 1:
            target = {0, (float)1.3+ i, 0};
            pole = {0, 0, 1};
            break;
            case 2:
            target = {cosf(ang)*2, 0.2, sinf(ang)*2};
            pole = {0, 0, 1};
            break;
            case 3:
            target ={-2.127778, 0.000000, -0.477778};
            break;
            }
            target = target*arm_len;
            
            phys_arm.resolve(target);

        } 

        BeginDrawing();
        ClearBackground((Color) {30, 30, 30, 255});
        BeginMode3D(cam);
        for (int j = 0; j < arm.bone_count; j++) { 
            DrawCubeWires(xn_to_rl(phys_arm.ideal_chain.positions[j]), 0.1, 0.1, 0.1, ORANGE);
            DrawCube(xn_to_rl(phys_arm.positions[j]), 0.1, 0.1, 0.1, RED);
        }

        for ( int j = 0; j < arm.bone_count - 2; j++) {
            DrawCube(xn_to_rl(phys_arm.ideal_chain.poles[j]), 0.05, 0.05, 0.05, GREEN );
        }

        DrawCube(xn_to_rl(target/arm_len), 0.05, 0.05, 0.05, BLUE);

        DrawGrid(10, 1);
        EndMode3D();

        draw_scene_min(&terminal); // do lua draw commands, draw terminal if open
        EndDrawing();
    }
    //--------------------------------------------------------------------------------------
    
    // Cleanup
    // --------------------------------------------------------------------------------------
#ifdef PROFILING
    ProfilerStop();                  // stop recording performance
#endif
    // CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
    
    for (int i =0; i< num_gamepad_threads; i++ )  {
        pthread_cancel(gamepad_thread[i]);
    } 
    set_interrupt(true);
    pthread_join(server_thread, NULL); 
    // pthread_cancel(pair_thread);
    // pthread_join(pair_thread, NULL); 
    destroy_msg_q();
    lua_close(L);                       
    reset_terminal_mode();
    CloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}