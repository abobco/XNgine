#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include "gamepad_server.h"

/* The following values come from include/joystick.h in the kernel source. */
#define JSIOCSBTNMAP_LARGE _IOW('j', 0x33, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#define JSIOCSBTNMAP_SMALL _IOW('j', 0x33, __u16[KEY_MAX_SMALL - BTN_MISC + 1])
#define JSIOCGBTNMAP_LARGE _IOR('j', 0x34, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#define JSIOCGBTNMAP_SMALL _IOR('j', 0x34, __u16[KEY_MAX_SMALL - BTN_MISC + 1])

int determine_ioctl(int fd, int *ioctls, int *ioctl_used, void *argp)
{
	int i, retval = 0;

	/* Try each ioctl in turn. */
	for (i = 0; ioctls[i]; i++) {
		if ((retval = ioctl(fd, ioctls[i], argp)) >= 0) {
			/* The ioctl did something. */
			*ioctl_used = ioctls[i];
			return retval;
		} else if (errno != -EINVAL) {
			/* Some other error occurred. */
			return retval;
		}
	}
	return retval;
}

int getbtnmap(int fd, uint16_t *btnmap)
{
	static int jsiocgbtnmap = 0;
	int ioctls[] = { JSIOCGBTNMAP, JSIOCGBTNMAP_LARGE, JSIOCGBTNMAP_SMALL, 0 };

	if (jsiocgbtnmap != 0) {
		/* We already know which ioctl to use. */
		return ioctl(fd, jsiocgbtnmap, btnmap);
	} else {
		return determine_ioctl(fd, ioctls, &jsiocgbtnmap, btnmap);
	}
}

int setbtnmap(int fd, uint16_t *btnmap)
{
	static int jsiocsbtnmap = 0;
	int ioctls[] = { JSIOCSBTNMAP, JSIOCSBTNMAP_LARGE, JSIOCSBTNMAP_SMALL, 0 };

	if (jsiocsbtnmap != 0) {
		/* We already know which ioctl to use. */
		return ioctl(fd, jsiocsbtnmap, btnmap);
	} else {
		return determine_ioctl(fd, ioctls, &jsiocsbtnmap, btnmap);
	}
}

int getaxmap(int fd, uint8_t *axmap)
{
	return ioctl(fd, JSIOCGAXMAP, axmap);
}

int setaxmap(int fd, uint8_t *axmap)
{
	return ioctl(fd, JSIOCSAXMAP, axmap);
}

#define NAME_LENGTH 128

char *axis_names[ABS_MAX + 1] = {
"X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder", 
"Wheel", "Gas", "Brake", "?", "?", "?", "?", "?",
"Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat3Y",
"?", "?", "?", "?", "?", "?", "?", 
};

/* These must match the constants in include/uapi/linux/input.h */
char *button_names[KEY_MAX - BTN_MISC + 1] = {
  /* BTN_0, 0x100, to BTN_9, 0x109 */
  "Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7", "Btn8", "Btn9",
  /* 0x10a to 0x10f */
  "?", "?", "?", "?", "?", "?",
  /* BTN_LEFT, 0x110, to BTN_TASK, 0x117 */
  "LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn", "TaskBtn",
  /* 0x118 to 0x11f */
  "?", "?", "?", "?", "?", "?", "?", "?",
  /* BTN_TRIGGER, 0x120, to BTN_PINKIE, 0x125 */
  "Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn",
  /* BTN_BASE, 0x126, to BASE6, 0x12b */
  "BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6",
  /* 0x12c to 0x12e */
  "?", "?", "?",
  /* BTN_DEAD, 0x12f */
  "BtnDead",
  /* BTN_A, 0x130, to BTN_TR2, 0x139 */
  "BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR", "BtnTL2", "BtnTR2",
  /* BTN_SELECT, 0x13a, to BTN_THUMBR, 0x13e */
  "BtnSelect", "BtnStart", "BtnMode", "BtnThumbL", "BtnThumbR",
  /* 0x13f */
  "?",
  /* Skip the BTN_DIGI range, 0x140 to 0x14f */
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  /* BTN_WHEEL / BTN_GEAR_DOWN, 0x150, to BTN_GEAR_UP, 0x151 */
  "WheelBtn", "Gear up",
};

int close_joystick_event_thread(pthread_t t_id) {
	gamepad_thread_interrupted = true;
	pthread_join(t_id, NULL);
	
	return 0;
}

/** pthread fn to read joystick events **/
void* joystick_event_thread(void *argv) {
	static int num_threads = 0;
	int fd, i;
	unsigned char axes = 2;
	unsigned char buttons = 2;
	int version = 0x000800;
	char name[NAME_LENGTH] = "Unknown";
	uint16_t btnmap[BTNMAP_SIZE];
	uint8_t axmap[AXMAP_SIZE];
	int btnmapok = 1;   

	// wait until a joystick connects
	int jid = num_threads;
	if ( argv != NULL ) 
		jid = (int) argv;
	else 
		num_threads++;
	char pipe_path[64] ="/dev/input/js";
	char str_num[2] = "";
	sprintf(str_num, "%d", jid);
	strcat(pipe_path, str_num);
    while ((fd = open(pipe_path, O_RDONLY)) < 0) {
		sleep(1);
	} 

    ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &buttons);
	ioctl(fd, JSIOCGNAME(NAME_LENGTH), name);

    getaxmap(fd, axmap);
	getbtnmap(fd, btnmap);

	printf("Driver version is %d.%d.%d.\n",
		version >> 16, (version >> 8) & 0xff, version & 0xff);

	/* Determine whether the button map is usable. */
	for (i = 0; btnmapok && i < buttons; i++) {
		if (btnmap[i] < BTN_MISC || btnmap[i] > KEY_MAX) {
			btnmapok = 0;
			break;
		}
	}
	if (!btnmapok) {
		/* btnmap out of range for names. Don't print any. */
		puts("jstest is not fully compatible with your kernel. Unable to retrieve button map!");
		printf("Joystick (%s) has %d axes ", name, axes);
		printf("and %d buttons.\n", buttons);
	} else {
		printf("Joystick (%s) has %d axes (", name, axes);
		for (i = 0; i < axes; i++)
			printf("%s%s", i > 0 ? ", " : "", axis_names[axmap[i]]);
		puts(")");

		printf("and %d buttons (", buttons);
		for (i = 0; i < buttons; i++) {
			printf("%s%s", i > 0 ? ", " : "", button_names[btnmap[i] - BTN_MISC]);
		}
		puts(").");
	}

    // event loop
	int id = insert_id( (void*) &fd );
    struct js_event js;
	struct Message msg_to_q = {         
				MSG_MOTION_VECTOR,  		
				0.0f,
				id,
				USER_GAMEPAD,
				(vec3) { 0, 0, 0 },      			
				// NULL            			
	};
	while (!gamepad_thread_interrupted) {
		if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
			printf("\n[GAMEPAD SERVER] Player %d disconnected (%s) \n", id, name);
			remove_id( (void*) &fd );
			return joystick_event_thread((void*)jid);	// wait for a new connection
		}	
	
		float nval = 10 * (float)js.value / SHRT_MAX;
		switch ( js.type ) {
			case 2: 
				msg_to_q.type = MSG_MOTION_VECTOR;
				switch ( js.number ) {
					case 0:
						msg_to_q.motion.x = nval;
					break;
					case 1:
						msg_to_q.motion.y = nval;
					break;
					default: break;
				} 
			break;
			
			case 1:
				switch ( js.number ) {
					case 0:
						msg_to_q.type = js.value ? MSG_BTN_A : MSG_BTN_A_UP;
					break;
				} 
			break;
		}
		enq_msg(msg_to_q);
	}

    return NULL;
}

#define PAIR_OUTPUT_LENGTH 4096

static int sync_interrupt = false;
void set_sync_interrupt(int b) {sync_interrupt = b;}
int get_sync_interrupt() {return sync_interrupt;}

void *sync_loop( void* argv ) {
	set_sync_interrupt(false);
	while ( !sync_interrupt ) {
		// create entrance/exit pipes
		int filedes[2];
		if (pipe(filedes) == -1) {
			perror("pipe");
			exit(1);
		}
		fcntl(filedes[0], F_SETFL, O_NONBLOCK);
		pid_t pid = fork();
		if ( pid == -1 ) {
			perror("fork");
			exit(1);
		} else if ( pid == 0 ) {
			// connect entrance
			printf("\n[GAMEPAD SERVER] Starting new pair process...\n");			
            while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}  // loop in case of SIGINT
			close(filedes[1]);
			close(filedes[0]);
			char *binaryPath = "python3";
			char *arg1 = "../bluetoothctlctl.py";
			
			execlp(binaryPath, binaryPath, arg1, NULL);
			perror("execl");
			_exit(1);
		}

        int nread; 
        char buf[PAIR_OUTPUT_LENGTH]; 

        close(filedes[1]); // write link 

        int done_reading = false;
        while ( !sync_interrupt && !done_reading) { 
            nread = read(filedes[0], buf, PAIR_OUTPUT_LENGTH); 
            switch (nread) { 
                case -1: // read error
                    if (errno == EAGAIN) { // pipe is empty b/c of nonblocking fncontrol
                        // printf("(pipe empty)\n"); 
                        sleep(1); 
                        break; 
                    } else {  
                        perror("read"); 
                        exit(4); 
                    } 

                case 0: // EOF reached
                    // printf("EOF\n"); 
                    close(filedes[0]); // read link 
                    done_reading = true;
                    break; 

                default: printf("%s\n", buf);  // text read 
            } 
        } 
        kill(pid, SIGKILL);

		wait(0);
	}

	return NULL;
}