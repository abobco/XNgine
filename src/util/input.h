#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define JOYSTICK_DEAD_ZONE 0.3
#define U_TURN_IGNORE_ANGLE 90

#define TURN_SENSITIVITY 10.0f;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#include <math.h>

#include "raylib.h"

#include "bob_math.h"
// #include "../snake.h"


enum GamepadType{
    GP_GENERIC,
    GP_JOYCON
};

enum RelevantKeys {
    VK_RETURN = 10,
    VK_BACKSPACE = 127,

    VK_SPECIAL_KEY_STARTING = 27,
    VK_UP = 65, 
    VK_DOWN,
    VK_RIGHT, 
    VK_LEFT, 
};

typedef struct TerminalInfo {
    int isOpen;
    int cursorPos;
    lua_State *L;
    // FILE *history;
    int hist_pos;
    const char *f_name;
    char *input;
} TerminalInfo;

TerminalInfo create_TerminalInfo( char *input_buffer, lua_State *L );

void reset_terminal_mode();

void set_conio_terminal_mode();

void init_conio_terminal(); 

int kbhit();

int getch();

float axis_to_angle(Vector2 axis);

int check_u_turn(Vector2 direction, Vector2 new_direction);

Vector2 gyro_to_axis(float angle);

void handle_keyboard_input(TerminalInfo *terminal);

void get_line( int line, TerminalInfo* terminal );

int lines_in_file( const char *file_name) ;

#ifdef __cplusplus
}
#endif