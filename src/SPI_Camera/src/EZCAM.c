#include <stdlib.h>
#include <stdio.h>
#include "ArduCAM.h"
#include "sccb_bus.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif
#include <pigpio.h>
#ifdef __cplusplus
}
#endif

#define PORT 8080 


#define NUM_GPIO 32
#define MIN_WIDTH (1000 - 450)
#define MAX_WIDTH (2000 + 450)

#define CAM_DEAD_ZONE 50
#define CAM_MID (320/2)

int run=1;
int i, g;

int step[NUM_GPIO];
int width[NUM_GPIO];
int used[NUM_GPIO];

void *cam_io_thread(void *arg);
void *servo_ctl_thread(void *arg);
void  dataParse(char* databuf);
void  INThandler(int sig);

int sockfd, newsockfd, portno, clilen;
struct sockaddr_in serv_addr, cli_addr;
struct sigaction sa;
unsigned char start_read_data = 0;
unsigned char read_data_OK = 0;

unsigned int length_cam1;

int sock = 0, valread; 

int target_x = 0;

void stop(int signum)
{
   run = 0;
}

int init_servo(int argc, char *argv[]) {
   if (gpioInitialise() < 0) return -1;

   gpioSetSignalFunc(SIGINT, stop);

   if (argc == 1) used[12] = 1;
   else
   {
      for (i=1; i<argc; i++)
      {
         g = atoi(argv[i]);
         if ((g>=0) && (g<NUM_GPIO)) used[g] = 1;
      }
   }

   printf("Sending servos pulses to GPIO");

   for (g=0; g<NUM_GPIO; g++)
   {
      if (used[g])
      {
         printf(" %d", g);
         step[g] = 25;
         width[g] = 1500;
      }
   }

   printf(", control C to stop.\n");
   return 0;
}

int main(int argc, char *argv[])
{
  pthread_t cam_io_tid, servo_ctl_tid; 
  pioInit();
  ArduCAM_CS_init( CAM_CS1, -1, -1, -1 );   // init the cs

  sccb_bus_init();
  spiInit(4000000, 0); //8MHZ
  Arducam_bus_detect( CAM_CS1, -1, -1, -1 );

  resetFirmware( CAM_CS1, -1, -1, -1 );  //reset the firmware
  ArduCAM_Init(sensor_model);
  signal(SIGINT, INThandler);
  
  // read camera command loop 
  start_read_data = 0;
  pthread_create(&cam_io_tid, NULL, cam_io_thread, NULL);
  pthread_create(&servo_ctl_tid, NULL, servo_ctl_thread, NULL);
  sleep(1);

  init_servo(argc, argv);

  struct sockaddr_in serv_addr; 
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
      printf("socket() failed:\n"); 
      return -1; 
  } 
  
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(PORT); 
      
  // Convert IPv4 and IPv6 addresses from text to binary form 
  if(inet_pton(AF_INET, "192.168.0.183", &serv_addr.sin_addr)<=0) { 
      printf("Invalid address\n"); 
      return -1; 
  } 
  
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  { 
      printf("connect() failed\n"); 
      return -1; 
  } 

  // support reconnection
  // sa.sa_handler = SIG_IGN;
  // sigaction( SIGPIPE, &sa, 0 );
  read_data_OK = 0;
  start_read_data = 0;

  // set camera resolution
  dataParse("ql=2");  

  start_read_data = 1;
  while(1)
  {
    static int prev_x_pos = 0;
    for (g=0; g<NUM_GPIO; g++)
    {
        if (used[g] && prev_x_pos != target_x )
        {
          if ( target_x < CAM_MID - CAM_DEAD_ZONE )
            width[g] += step[g];
          else if ( target_x > CAM_MID + CAM_DEAD_ZONE )
            width[g] -= step[g];

          if ((width[g]<MIN_WIDTH) || (width[g]>MAX_WIDTH))
          {
              // step[g] = -step[g];
              width[g] -= step[g];
          }

          gpioServo(g, width[g]);
        }
    }
    prev_x_pos = target_x;
    time_sleep(0.25);
  }
  pthread_join(cam_io_tid, NULL); // wait on io thread
  pthread_join(servo_ctl_tid, NULL); // wait on io thread
  return 0;
}

void *servo_ctl_thread(void *arg) {
  while (1) {
    if ( sock ) {
      uint32_t tmp, x_pos;
      uint32_t left = sizeof(tmp);
      char *buf = (char *) &tmp;
      int rc;
      do {
        rc =  recv(sock, buf, left, 0);
        buf += rc;
        left -= rc;
      } while (left > 0);
      x_pos = ntohl(tmp);
      target_x = x_pos;
      // printf("%d\n", x_pos);
    }
  } 
}

void *cam_io_thread(void *arg) {
  while (1) {
    if (start_read_data == 1) {
      singleCapture(CAM_CS1);
	  
      sendbuf_cam1 = readbuf;
      length_cam1 = length;
      // printf("length: %d\n", length_cam1);

      // send frame over TCP socket
      if (sock) { 
        // send buffer size 1st cuz jpeg compression makes the size unpredictable 
        uint32_t send_len = length_cam1;
        uint32_t tmp = htonl(send_len);
        send(sock, &tmp, sizeof(tmp), 0);

        // send frame buffer
        send(sock, sendbuf_cam1, length_cam1, 0);
        
        usleep(150e3);
      } 
    }
  }
  return 0;
}

void  INThandler(int sig) {
  signal(sig, SIG_IGN); 	
  if (newsockfd < 0) {
    close(newsockfd);
    close(sockfd);
  }
  close(sock);
	start_read_data = 0;
  printf("\nlater nerd\n");
  exit(0);
}

// i sure as hell didnt write this shit
void  dataParse(char* databuf) {
  start_read_data = 1;
  if (strstr(databuf, "ql=0")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_160x120);
      printf("Set the resolution to OV2640_160x120 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_320x240);
      printf("Set the resolution to OV5640_320x240 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_320x240);
      printf("Set the resolution to OV5642_320x240 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=1")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_176x144);
      printf("Set the resolution to OV2640_176x144 successfully\r\n");

    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_352x288);
      printf("Set the resolution to OV5640_352x288 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_640x480);
      printf("Set the resolution to OV5642_640x480 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=2")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_320x240);
      printf("Set the resolution to OV2640_320x240 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_640x480);
      printf("Set the resolution to OV5640_640x480 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_1024x768);
      printf("Set the resolution to OV5642_1024x768 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=3")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_352x288);
      printf("Set the resolution to OV2640_352x288 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_800x480);
      printf("Set the resolution to OV5640_800x480 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_1280x960);
      printf("Set the resolution to OV5642_1280x960 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=4") ) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_640x480);
      printf("Set the resolution to OV2640_640x480 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_1024x768);
      printf("Set the resolution to OV5640_1024x768 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_1600x1200);
      printf("Set the resolution to OV5642_1600x1200 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=5")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_800x600);
      printf("Set the resolution to OV2640_800x600 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_1280x960);
      printf("Set the resolution to OV5640_1280x960 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_2048x1536);
      printf("Set the resolution to OV5642_2048x1536 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=6")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_1024x768);
      printf("Set the resolution to OV2640_1024x768 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_1600x1200);
      printf("Set the resolution to OV5640_1600x1200 successfully\r\n");
    }
    else if (sensor_model == OV5642) {
      OV5642_set_JPEG_size(OV5642_2592x1944);
      printf("Set the resolution to OV5642_2592x1944 successfully\r\n");

    }
  }
  else if (strstr(databuf, "ql=7")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_1280x1024);
      printf("Set the resolution to OV2640_1280x1024 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_2048x1536);
      printf("Set the resolution to OV5640_2048x1536 successfully\r\n");
    }
  }
  else if (strstr(databuf, "ql=8")) {
    if (sensor_model == OV2640) {
      OV2640_set_JPEG_size(OV2640_1600x1200);
      printf("Set the resolution to OV2640_1600x1200 successfully\r\n");
    }
    else if (sensor_model == OV5640) {
      OV5640_set_JPEG_size(OV5640_2592x1944);
      printf("Set the resolution to OV5640_2592x1944 successfully\r\n");
    }
  }
}



