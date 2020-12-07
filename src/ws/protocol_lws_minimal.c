/*
 * ws protocol handler plugin for "lws-minimal"
 * 
 * Adapted from an 8-message ringbuffer example originally written by the author of libwebsockets, Andy Green (andy@warmcat.com).
 * Additional functionality added by me (Austin Bobco):
 * 		- Insert messages into a global message queue, together w/ Bluetooth gamepad events
 * 		- Message exchange where the server sends a player id to newly connected clients
 * 		- Message types
 * 
 */
#include <time.h>

#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <string.h>
#include "message_queue.h"
#include "ws_common.h"
#include "lws_gibberish.c"

#define RING_BUFFER_LENGTH 8
#define INT_TO_FLOAT_CONV_FACTOR 1E-7


// read in 3 integers representing device motion for a player,
// convert to a vector of floats
int ws_recv_motion_event(int32_t* packet) {
	// convert to game data
	int32_t player = packet[1]-1;
	motion_msg[player].x = ((float)packet[2])*INT_TO_FLOAT_CONV_FACTOR;
	motion_msg[player].y = ((float)packet[3])*INT_TO_FLOAT_CONV_FACTOR;
	motion_msg[player].z = ((float)packet[4])*INT_TO_FLOAT_CONV_FACTOR;

	// new message to add to the queue
	struct Message msg_to_queue = {         
		MSG_MOTION_VECTOR,  		// message type
		0.0f,
		player,
		USER_WEBSOCKET,
		motion_msg[player],      // motion data    (optional)
		// NULL            			// string message (optional)
	};
	enq_msg(msg_to_queue); 

	return 0;
}

// read the message type, pass it to the appropriate handler
int ws_recv_msg(void *input, size_t len) {
	struct msg amsg;
	// over-allocate the message to make room for websocket headers
	amsg.payload = malloc(LWS_PRE + len);
	amsg.len = len;
	if (!amsg.payload) {
		lwsl_user("OOM: dropping\n");
		return 1;
	}

	// copy the message into the payload as 32 bit ints
    memcpy((int32_t *)amsg.payload, input, len); 

	// for some reason we don't have to convert from network byte order here
    // in fact if we do call ntohl(), we will get garbage numbers 
    int32_t* packet = (int32_t*)(amsg.payload);

	int32_t value = packet[0];
	int32_t player = packet[1]-1;

	// switch ( type ) {
	// 	case MSG_MOTION_VECTOR:
	// 		ws_recv_motion_event(packet);
	// 	break;

	// 	case MSG_BTN_A:
	// 	case MSG_BTN_B:
	// 	case MSG_BTN_A_UP:
	// 	case MSG_BTN_B_UP:
	// 	case MSG_QUIT:
	// 	// Clang gives a compiler error if the 1st statement after a switch case is not an expression,
	// 	// b/c this language was made for cyborgs. We can get around this by wrapping the entire
	// 	// block in these dumbass braces
		// {
			// virtual button press event
			struct Message msg_to_queue = {         
				value,  			// message type
				0.0f,						// timestamp
				player,							// user id
				USER_WEBSOCKET,
				{0,0,0},      					// motion data    (optional)
				// NULL            			// string message (optional)
			};
			enq_msg(msg_to_queue); 		
		// }
		// break;		
	// }
	
	free(amsg.payload);
	return 0;
}

// send one 32 bit int to all connections
int ws_broadcast_int(struct per_vhost_data__minimal *virtualhost, int32_t number) {
	// outgoing message
	struct msg amsg;

	// over-allocate the message to make room for websocket headers
	amsg.len = sizeof ( number );
	amsg.payload = malloc(LWS_PRE + amsg.len);
	if (!amsg.payload) {
		lwsl_user("OOM: dropping\n");
		return 1;
	}

	// write the number into the message starting after the websocket headers
	memcpy(LWS_PRE+ (unsigned char* )amsg.payload, (void*)(&number), amsg.len);

	// signal that we want to write to all connections when available
	ws_broadcast(virtualhost, amsg);
	return 0;
}

static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct per_session_data__minimal *pss =
			(struct per_session_data__minimal *)user;
	struct per_vhost_data__minimal *vhd =
			(struct per_vhost_data__minimal *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
					
	const struct msg *pmsg;		// outgoing message
	int n, m;

	switch (reason) {
	// ----------------------- start listening for connections ----------------------------------------------------
		case LWS_CALLBACK_PROTOCOL_INIT:
			// print allocation info
			lwsl_user("Protocol Init: allocating %d bytes\n", 
				sizeof(struct per_vhost_data__minimal) + sizeof(struct msg)*RING_BUFFER_LENGTH);

			// allocate the protocol's per-virtualhost memory
			vhd = (struct per_vhost_data__minimal *)lws_protocol_vh_priv_zalloc(
					lws_get_vhost(wsi),						// vhost to allocate
					lws_get_protocol(wsi),					// protocol for the instance
					sizeof(struct per_vhost_data__minimal)	// bytes to allocate
			);
			// get info for the websocket context
			vhd->context = lws_get_context(wsi);	
			vhd->protocol = lws_get_protocol(wsi);	
			vhd->vhost = lws_get_vhost(wsi);

			// allocate the ring buffer for messages
			vhd->ring = lws_ring_create(
							sizeof(struct msg), 			// size of 1 element in bytes
							RING_BUFFER_LENGTH,				// max # of elements in the buffer
							__minimal_destroy_message);		// callback for when an element is removed			
			if (!vhd->ring)
				return 1;
			break;
		// free the msg buffer when we're done
		case LWS_CALLBACK_PROTOCOL_DESTROY:
			lwsl_user("destroying context\n");
			lws_ring_destroy(vhd->ring);
			break;
	// ------------------------------------------------------------------------------------------------------------

	// ----------------------------------- open/close connections  ------------------------------------------------
		case LWS_CALLBACK_ESTABLISHED:
			/* add ourselves to the list of live pss held in the vhd */
			lwsl_user("LWS_CALLBACK_ESTABLISHED: wsi %p\n", wsi);
			lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
			pss->tail = lws_ring_get_oldest_tail(vhd->ring);
			pss->wsi = wsi;
		
			lwsl_user("Printing connections: \n");
			lws_start_foreach_llp(struct per_session_data__minimal **,
							ppss, vhd->pss_list) {	
				lwsl_user("wsi %p\n", (*ppss)->wsi );
			} lws_end_foreach_llp(ppss, pss_list);

			ws_broadcast_int(vhd, insert_id( (void*)wsi ) +1 );
			break;
		
		case LWS_CALLBACK_CLOSED:{
			lwsl_user("LWS_CALLBACK_CLOSED: wsi %p\n", wsi);
			/* remove our closing pss from the list of live pss */
			lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
					pss, vhd->pss_list);

			remove_id((void*)wsi);	

			// new message to add to the queue
			struct Message msg_to_queue = {         
				MSG_DISCONNECT,  			// message type
				0.0f,						// timestamp
				0,							// user id
				USER_WEBSOCKET,
				{0,0,0}      					// motion data    (optional)
				// NULL            			// string message (optional)
			};
				
			enq_msg(msg_to_queue);
			
			break;
		}
	// ------------------------------------------------------------------------------------------------------------

	// -------------------------------------- send a message ------------------------------------------------------
		case LWS_CALLBACK_SERVER_WRITEABLE:
			// check if the connection is still active
			if (pss->culled)
				break;
			// get the outgoing message from the ring buffer
			pmsg = (const struct msg *)lws_ring_get_element(vhd->ring, &pss->tail);
			if (!pmsg)
				break;

			// write the darn message to the socket
			m = ws_write(vhd, pss, wsi, pmsg, LWS_WRITE_BINARY);

			/* more to do for us? */
			if (lws_ring_get_element(vhd->ring, &pss->tail))
				/* come back as soon as we can write more */
				lws_callback_on_writable(pss->wsi);
			break;
	// ------------------------------------------------------------------------------------------------------------

	// ------------------------------------ read a message --------------------------------------------------------
		case LWS_CALLBACK_RECEIVE:
			n = (int)lws_ring_get_count_free_elements(vhd->ring);
			if (!n) {
				/* forcibly make space */
				cull_lagging_clients(vhd);
				n = (int)lws_ring_get_count_free_elements(vhd->ring);
			}
			if (!n)
				break;

			// lwsl_user("LWS_CALLBACK_RECEIVE: free space %d\n", n);

			// read the message 
			ws_recv_msg(in, len);
			break;
	// ------------------------------------------------------------------------------------------------------------
		default:
			break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"lws-minimal", \
		callback_minimal, \
		sizeof(struct per_session_data__minimal), \
		0, \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)

/* boilerplate needed if we are built as a dynamic plugin */

static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_MINIMAL
};

int
init_protocol_minimal(struct lws_context *context,
		      struct lws_plugin_capability *c)
{
	if (c->api_magic != LWS_PLUGIN_API_MAGIC) {
		lwsl_err("Plugin API %d, library API %d", LWS_PLUGIN_API_MAGIC,
			 c->api_magic);
		return 1;
	}

	c->protocols = protocols;
	c->count_protocols = LWS_ARRAY_SIZE(protocols);
	c->extensions = NULL;
	c->count_extensions = 0;

	return 0;
}

int
destroy_protocol_minimal(struct lws_context *context)
{
	return 0;
}
#endif
