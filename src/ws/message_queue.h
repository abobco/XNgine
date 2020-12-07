#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include <stdint.h>

#define MAX_CONNECTIONS 16
#define RING_BUFFER_LENGTH 8
#define INT_TO_FLOAT_CONV_FACTOR 1E-7

// ------------- phone message stuff ------------------------------------------

typedef struct vec3 {
    float x;
    float y;
    float z;
} vec3;

 enum MSG_TYPE {
    MSG_DISCONNECT = -1,
    MSG_CONNECT,
    MSG_MOTION_VECTOR,
    MSG_STRING,
    MSG_QUIT,
    MSG_BTN_A,
    MSG_BTN_B,
    MSG_BTN_A_UP,
    MSG_BTN_B_UP
} ;

enum USER_TYPE {
    USER_WEBSOCKET,
    USER_GAMEPAD
};

typedef struct Message {
    int32_t value;
    float timestamp;
    int user_id;
    enum USER_TYPE user_type;
    vec3 motion;
    // const char* str_msg;
    // int button;
} Message;

typedef struct MessageList {
    Message* buffer;
    int length;
} MessageList;

static void *connection_ids_new[MAX_CONNECTIONS + 4];
void **get_connection_ids();
int insert_id( void* id ) ;
void remove_id( void *id ) ;

static vec3 motion_msg[MAX_CONNECTIONS];
static int32_t num_connections = 0;

#define MAX_MSG_BUFFER_SIZE 512

static pthread_mutex_t binary_sem;
static struct queue *message_queue;

int ws_create_thread(char *domain, int port  );
vec3 get_motion_data(int idx );
int get_connections();
int get_message_queue_size();
struct MessageList pop_new_messages();
void add_connection();
int get_connections();
void enq_msg(Message msg);
void init_msg_q();
void destroy_msg_q();
// ------------------------------------------------------------------------

/* standard linked list element */
struct elt {
    struct elt *next;
    struct Message msg;
};

struct queue {
    struct elt *head;  /* dequeue this next */
    struct elt *tail;  /* enqueue after this */
    int length;
};

/* create a new empty queue */
struct queue *queueCreate(void);

/* add a new value to back of queue */
void enq(struct queue *q, struct Message msg);

int queueEmpty(const struct queue *q);

/* remove and return value from front of queue */
struct Message deq(struct queue *q);

/* print contents of queue on a single line, head first */
void queuePrint(struct queue *q);

/* free a queue and all of its elements */
void queueDestroy(struct queue *q);

#ifdef __cplusplus
}
#endif