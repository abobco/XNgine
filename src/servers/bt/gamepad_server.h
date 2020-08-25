#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <linux/input.h>
#include <pthread.h>
#include "../message_queue.h"

/* The following values come from include/input.h in the kernel
   source; the small variant is used up to version 2.6.27, the large
   one from 2.6.28 onwards. We need to handle both values because the
   kernel doesn't; it only expects one of the values, and we need to
   determine which one at run-time. */
#define KEY_MAX_LARGE 0x2FF
#define KEY_MAX_SMALL 0x1FF

/* Axis map size. */
#define AXMAP_SIZE (ABS_MAX + 1)

/* Button map size. */
#define BTNMAP_SIZE (KEY_MAX_LARGE - BTN_MISC + 1)

/* Retrieves the current axis map in the given array, which must
   contain at least AXMAP_SIZE elements. Returns the result of the
   ioctl(): negative in case of an error, 0 otherwise for kernels up
   to 2.6.30, the length of the array actually copied for later
   kernels. */
int getaxmap(int fd, uint8_t *axmap);

/* Uses the given array as the axis map. The array must contain at
   least AXMAP_SIZE elements. Returns the result of the ioctl():
   negative in case of an error, 0 otherwise. */
int setaxmap(int fd, uint8_t *axmap);

/* Retrieves the current button map in the given array, which must
   contain at least BTNMAP_SIZE elements. Returns the result of the
   ioctl(): negative in case of an error, 0 otherwise for kernels up
   to 2.6.30, the length of the array actually copied for later
   kernels. */
int getbtnmap(int fd, uint16_t *btnmap);

/* Uses the given array as the button map. The array must contain at
   least BTNMAP_SIZE elements. Returns the result of the ioctl():
   negative in case of an error, 0 otherwise. */
int setbtnmap(int fd, uint16_t *btnmap);

static int gamepad_thread_interrupted = false;  // thread control variable 4 dummies

int close_joystick_event_thread(pthread_t t_id);

void *joystick_event_thread(void *argv);        // pthread fn to read joystick events

void set_sync_interrupt(int b);
int get_sync_interrupt();
void *sync_loop( void* argv );

#ifdef __cplusplus
}
#endif