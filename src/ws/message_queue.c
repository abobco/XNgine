#include "message_queue.h"
#include <stdbool.h> 

void add_connection() {
    num_connections++;
}
int get_connections() {
    return num_connections;
}
void init_msg_q() {
    message_queue = queueCreate();
	pthread_mutex_init(&binary_sem, NULL);
}

void destroy_msg_q() {
    queueDestroy(message_queue);
    pthread_mutex_destroy(&binary_sem);
}

int get_message_queue_size() {
	return message_queue->length;
}

void **get_connection_ids() {
    return connection_ids_new;
}

int insert_id( void *id ) {
    static bool firstTime = true;
    if ( firstTime ) {
        firstTime = false;
        for ( int i = 0; i < MAX_CONNECTIONS + 4; i++ )
            connection_ids_new[i] = NULL;
    }
    int index = -1;
    for ( int i =0; i <  MAX_CONNECTIONS + 4; i++ ) {
        if ( connection_ids_new[i] == NULL ) {
            connection_ids_new[i] = id;
            index = i;
            break;
        }
    }
    int new_connection_count = 0;
    for ( int i = 0; i < MAX_CONNECTIONS + 4; i++ ) {
        if ( connection_ids_new[i] != NULL )
            new_connection_count++;
    }
    num_connections = new_connection_count;
    printf("new connection count: %d\n", num_connections);
    return index;
}

void remove_id( void *id ) {
    for ( int i =0; i <  MAX_CONNECTIONS + 4; i++ ) {
        if ( connection_ids_new[i] == id ) {
            connection_ids_new[i] = NULL;
            num_connections--;
            break;
        }
    }
}

// thread-safe message insertion
void enq_msg(Message msg) {
    pthread_mutex_lock(&binary_sem);    
    enq(message_queue, msg); 
    pthread_mutex_unlock(&binary_sem); 
}

MessageList pop_new_messages() {
    static Message buffer[MAX_MSG_BUFFER_SIZE];
    static MessageList new_messages = {buffer, 0};

    pthread_mutex_lock(&binary_sem); 

    // copy the messages
    new_messages.length = message_queue->length;
    for ( int i =0; i < (new_messages.length < MAX_MSG_BUFFER_SIZE ? new_messages.length : MAX_MSG_BUFFER_SIZE); i++) {
        new_messages.buffer[i] = deq(message_queue); // pop each element
        
        // printf( "%f, %f, %f\n", new_messages.buffer[i].motion.x, new_messages.buffer[i].motion.y, new_messages.buffer[i].motion.z);
    }
    pthread_mutex_unlock(&binary_sem);
    return new_messages;
}
/* create a new empty queue */
struct queue *
queueCreate(void)
{
    struct queue *q;

    q = malloc(sizeof(struct queue));

    q->head = q->tail = 0;
    q->length = 0;

    return q;
}

/* add a new value to back of queue */
void
enq(struct queue *q, struct Message msg)
{
    struct elt *e;

    e = malloc(sizeof(struct elt));
    assert(e);

    e->msg = msg;

    /* Because I will be the tail, nobody is behind me */
    e->next = 0;

    if(q->head == 0) {
        /* If the queue was empty, I become the head */
        q->head = e;
    } else {
        /* Otherwise I get in line after the old tail */
        q->tail->next = e;
    }

    /* I become the new tail */
    q->tail = e;

    q->length++;
}

int
queueEmpty(const struct queue *q)
{
    return (q->head == 0);
}

/* remove and return value from front of queue */
struct Message
deq(struct queue *q)
{
    struct Message ret;
    struct elt *e;

    assert(!queueEmpty(q));

    ret = q->head->msg;

    /* patch out first element */
    e = q->head;
    q->head = e->next;

    q->length--;

    free(e);

    return ret;
}

/* print contents of queue on a single line, head first */
void
queuePrint(struct queue *q)
{
    struct elt *e;

    for(e = q->head; e != 0; e = e->next) {
        printf("(%f, %f) ", e->msg.motion.x, e->msg.motion.y);
    }
    
    putchar('\n');
}

/* free a queue and all of its elements */
void
queueDestroy(struct queue *q)
{
    while(!queueEmpty(q)) {
        deq(q);
    }

    free(q);
}