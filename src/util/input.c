#include "input.h"
#include "raylib.h"

// mobile gyroscope event handlers
Vector2 gyro_to_axis(float angle) {
    Vector2 axis;

    axis.x = cosf(PI*0.5f+angle*PI/180.0f);
    axis.y = sinf(PI*0.5f+angle*PI/180.0f);

    return axis;
}

int check_u_turn(Vector2 direction, Vector2 new_direction) {
    Vector2 inv_direction = (Vector2) {direction.x*-1.0f, direction.y*-1.0f};

    if ( new_direction.x < inv_direction.x + EPSILON && new_direction.x > inv_direction.x - EPSILON
        && new_direction.y < inv_direction.y + EPSILON && new_direction.y > inv_direction.y - EPSILON)
        return 1;
    return 0;
}

// converts a normalized joystick axis coordinate to 0-360 degree angle
// there's definitely better ways to do this but this shit works 
float axis_to_angle(Vector2 axis) {
    float angle = 0;     
    float flipped_y_axis = axis.y*-1.0f;

    // joystick is pressed out diagonally
    if ( axis.x > 0 && axis.y < 0 )        // quad 1
        angle = atan(flipped_y_axis/axis.x);
    else if ( axis.x < 0 && axis.y < 0 )   // quad 2
        angle = PI + atan(flipped_y_axis/axis.x);
    else if ( axis.x < 0 && axis.y > 0 )   // quad 3
        angle = PI + atan(flipped_y_axis/axis.x);
    else if ( axis.x > 0 && axis.y > 0 )   // quad 4
        angle = 2.0*PI + atan(flipped_y_axis/axis.x); 
    
    // joystick is very close to an axis
    else if ( fabsf(axis.x) < EPSILON && axis.y < -1.0f + EPSILON )
        angle = PI*0.5;
    else if ( axis.x < -1.0f + EPSILON && fabsf(axis.y) < EPSILON)
        angle = PI;
    else if ( fabsf(axis.x) < EPSILON && axis.y > 1.0f - EPSILON)
        angle = PI*1.5;
    else if ( axis.x > 1.0f - EPSILON && fabsf(axis.y) < EPSILON )
        angle = 0.0f;

    return angle * 180.0/PI + 90; // convert to degrees and orient to match the snake
}

// ssh keyboard event handlers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

static struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(0, TCSAFLUSH, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    // cfmakeraw(&new_termios);
    new_termios.c_lflag &= ~ECHO;   // turn off echo
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

TerminalInfo create_TerminalInfo(  lua_State *L ) {
    char *ib = calloc( 512, sizeof(char));
    TerminalInfo t = {
        false,
        0,
        L,
        0,
        "terminal_history.txt",
        ib,
    };
    
    return t;
}

static void print_terminal(TerminalInfo *terminal) {
#if DEBUG_TERMINAL         
    printf("[LUA CONSOLE]:\n > %s\n", terminal->input);
#else                    // 
    printf("%c[2J[LUA CONSOLE]:\n > %s\n", 27, terminal->input);
#endif
}

 void handle_keyboard_input(TerminalInfo *terminal) {
    if ( kbhit() ) {
        char c = getch();
        // PRINT(c);
        if ( !terminal->isOpen) {
            switch (c) {
                case '`': 
                    terminal->isOpen = true; // send keyboard input to lua console
#if DEBUG_TERMINAL
                    printf("\n[LUA CONSOLE]:\n > \n");  
#else
                    printf("\n%c[2J[LUA CONSOLE]:\n > \n", 27);    
#endif              
                break;

                case 'c':
                    CloseWindow();
                break;
            }   
        } else {
            static int special_keymode = false;
            if (c == VK_SPECIAL_KEY_STARTING) {
                special_keymode = true;
            } else if ( special_keymode ) {
                switch (c) {
                    case VK_RIGHT: case VK_LEFT: case VK_UP: case VK_DOWN:
                        if ( c == VK_LEFT && terminal->cursorPos > 0)
                            terminal->cursorPos--;
                        if ( c == VK_RIGHT && terminal->cursorPos < strlen(terminal->input))
                            terminal->cursorPos++;
                        if ( c == VK_UP ) {
                            get_line(++terminal->hist_pos, terminal);
                            terminal->cursorPos = strlen(terminal->input);
                            print_terminal(terminal);
                        } 
                        if ( c == VK_DOWN && terminal->hist_pos > 0) {
                            get_line(--terminal->hist_pos, terminal);
                            terminal->cursorPos = strlen(terminal->input);
                            print_terminal(terminal);
                        }

                        special_keymode = false;
                        return;
                }
            }
            if ( !special_keymode ) {
                if ( c == VK_RETURN ) {
                    FILE *hist = fopen(terminal->f_name, "a+");
                    check_lua(terminal->L, luaL_dostring(terminal->L, terminal->input));

                    fprintf( hist, "%s\n", terminal->input );
                    fclose(hist);
                    memset(terminal->input, 0, 512);
                    terminal->isOpen = false;
                    terminal->cursorPos = 0;
                    terminal->hist_pos = 0;
                } else {
                    if ( c == VK_BACKSPACE && terminal->cursorPos > 0) {
                        for ( int i = --terminal->cursorPos; i < strlen(terminal->input); i++ ) {
                            terminal->input[i] = terminal->input[i+1];
                        }
                    }
                    else if ( c != VK_BACKSPACE ) {
                        terminal->input[strlen(terminal->input)+1] = '\0';
                        for ( int i = strlen(terminal->input);  i > terminal->cursorPos;  i-- ) {
                            terminal->input[i] = terminal->input[i-1];
                        }
                        terminal->input[terminal->cursorPos++] = c; 
                    }
                    print_terminal(terminal);
                }
            }
        }
    }
}
