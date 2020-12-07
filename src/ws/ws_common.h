// websocket libraries and types for this project
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <libwebsockets.h>
#include "message_queue.h"

#define LWS_PLUGIN_STATIC

static int interrupted;

void set_interrupt(int i);

typedef struct Socket {
    struct lws_context_creation_info info;
    struct lws_context *context;
    int n;
    int logs;
} Socket;

typedef struct ServerOptions {
    // const char* mount_dir;
    const char* web_domain;
    int port;
} ServerOptions;

/* one of these created for each message */

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
	struct per_session_data__minimal *pss_list;
	struct lws *wsi;
	uint32_t tail;

	unsigned int culled:1;
};

struct lws *connection_ids[MAX_CONNECTIONS];

/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct per_session_data__minimal *pss_list; /* linked-list of live pss */

	struct lws_ring *ring; /* ringbuffer holding unsent messages */
};


unsigned long long ms_since_epoch();

#ifdef __cplusplus
}
#endif