#ifndef __WS_RING_SERVER__
#define __WS_RING_SERVER__
#include "ws_common.h"
#include "protocol_lws_minimal.c"

#define MY_DOMAIN "www.studiostudios.net"
#define MY_MOUNT "../client"

Socket start_server();

static struct lws_protocols protocols[] = {
	{ "http", lws_callback_http_dummy, 0, 0 },
	LWS_PLUGIN_PROTOCOL_MINIMAL,
	{ NULL, NULL, 0, 0 } /* terminator */
};

static const struct lws_http_mount mount = {
	/* .mount_next */		NULL,		/* linked-list "next" */
	/* .mountpoint */		"/",		/* mountpoint URL */
	/* .origin */			MY_MOUNT, /* serve from dir */
	/* .def */				"index.html",	/* default filename */
	/* .protocol */			NULL,
	/* .cgienv */			NULL,
	/* .extra_mimetypes */	NULL,
	/* .interpret */		NULL,
	/* .cgi_timeout */		0,
	/* .cache_max_age */		0,
	/* .auth_mask */		0,
	/* .cache_reusable */		0,
	/* .cache_revalidate */		0,
	/* .cache_intermediaries */	0,
	/* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
	/* .mountpoint_len */		1,		/* char count */
	/* .basic_auth_login_file */	NULL,
};

void sigint_handler(int sig)
{
	interrupted = 1;
}

unsigned long long ms_since_epoch() {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (unsigned long long)(tv.tv_sec)  * 1000 +
		   (unsigned long long)(tv.tv_usec) / 1000;

}

void *start_server_loop(void* argv) {
    Socket server_sock = start_server();

	while (server_sock.n >= 0 && !interrupted)
		server_sock.n = lws_service(server_sock.context, 0);

	printf("destroying context\n");
	lws_context_destroy(server_sock.context);  // close websocket context

	printf("destroying queue\n");
	queueDestroy(message_queue);
	printf("queue destroyed\n");
	pthread_mutex_destroy(&binary_sem);
    return NULL;
}

int ws_create_thread(char *domain, int port  ) {
	if ( domain == NULL )
		domain = MY_DOMAIN;
	if ( port == 0 )
		port = 3000;

	// create message & mutex lock
	message_queue = queueCreate();
	pthread_mutex_init(&binary_sem, NULL);

	struct ServerOptions s = { domain, port };
	pthread_t thread_id;
    pthread_create(&thread_id, NULL, &start_server_loop, (void*) &s );   
    return (int)thread_id; 
}

Socket start_server()
{
    Socket server_sock;
	struct lws_context_creation_info info;
	struct lws_context *context;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;

	signal(SIGINT, sigint_handler);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal ws server (lws_ring) | visit http://10.0.0.2:7681\n");

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = 3000;
	info.mounts = &mount;
	info.protocols = protocols;
	info.vhost_name = MY_DOMAIN;
	info.options =
		LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	#if defined(LWS_WITH_TLS)
        // get the path to the cert for the given website
        char ssl_path[256]= "/etc/letsencrypt/live/";
        char ssl_cert[256], ssl_key[256];
        // append the domain to the path
        strcat(ssl_path, MY_DOMAIN); 
        strcpy(ssl_cert, ssl_path);
        strcpy(ssl_key, ssl_path);
        // append the cert and key files
        strcat(ssl_cert, "/fullchain.pem");
        strcat(ssl_key, "/privkey.pem");
        
        // pass ssl info to lws
        lwsl_user("Server using TLS\n");
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.ssl_cert_filepath = ssl_cert;
        info.ssl_private_key_filepath = ssl_key;
	#endif

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
	//	return 1;
	}

	server_sock.info = info;
	server_sock.context = context;
	server_sock.n = n;
	server_sock.logs = logs;
    return server_sock;
}

vec2 get_motion_data(int idx ) {
	return motion_msg[idx];
}

#endif