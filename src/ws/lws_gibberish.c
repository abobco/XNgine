#include "ws_common.h"
#include <sys/time.h>

// free the contents of a given message from the heap
static void __minimal_destroy_message(void *_msg) {
	struct msg *msg = (struct msg*)_msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}

// write data of any type to the given socket
// 
// NOTE: this function should only be called inside of callback_minimal(),
// which is in protocol_lws_minimal.c
int ws_write(
	struct per_vhost_data__minimal *virtualhost,
 	struct per_session_data__minimal *receiver,
	struct lws *wsi, 			
	const struct msg *pmsg, 
	enum lws_write_protocol type ) 
{
	// print the integer that we are about to send
	if ( type == LWS_WRITE_BINARY ) {
		// ?????????????????????????????????????????????????????????
		int32_t SIMPLE_FKIN_INT = *(int32_t*)(((unsigned char *)pmsg->payload) + LWS_PRE);
		struct timeval tv;
		gettimeofday(&tv, NULL);
		unsigned long long millisecondsSinceEpoch =
			(unsigned long long)(tv.tv_sec) * 1000 +
			(unsigned long long)(tv.tv_usec) / 1000;
		
		lwsl_user( "Sending int: %d at t=%llu\n", SIMPLE_FKIN_INT, millisecondsSinceEpoch );
	}
	// write the integer to the socket
	int bytes_written = lws_write(wsi, ((unsigned char *)pmsg->payload) +
			LWS_PRE, pmsg->len, type);
	if (bytes_written < (int)pmsg->len) {
		lwsl_err("ERROR %d writing to ws socket\n", bytes_written);
		return -1;
	}

	lws_ring_consume_and_update_oldest_tail(
		virtualhost->ring,	/* lws_ring object */
		struct per_session_data__minimal, /* type of objects with tails */
		&receiver->tail,	/* tail of guy doing the consuming */
		1,		/* number of payload objects being consumed */
		virtualhost->pss_list,	/* head of list of objects with tails */
		tail,		/* member name of tail in objects with tails */
		pss_list	/* member name of next object in objects with tails */
	);

	return bytes_written;
}

// send a message into the outgoing message buffer for all connections
int ws_broadcast(struct per_vhost_data__minimal *vhd, struct msg amsg) {
	// put the message into the ring buffer
	if (!lws_ring_insert(vhd->ring, &amsg, 1)) {
		__minimal_destroy_message(&amsg);
		lwsl_user("dropping!\n");
		return 1;
	}

	// write to all socket connections when available
	lws_start_foreach_llp(struct per_session_data__minimal **,
					ppss, vhd->pss_list) {
		lws_callback_on_writable((*ppss)->wsi);
	} lws_end_foreach_llp(ppss, pss_list);

	return 0;
}

// send a message to one client of the given virtual host
int ws_send(struct per_session_data__minimal *receiver, struct per_vhost_data__minimal *vhd, struct msg *amsg) {
	// put the message into the ring buffer
	if (!lws_ring_insert(vhd->ring, amsg, 1)) {
		__minimal_destroy_message(amsg);
		lwsl_user("dropping!\n");
		return 1;
	}

	// write to the socket connection when available
	lws_callback_on_writable((receiver)->wsi);

	return 0;
}

static void cull_lagging_clients(struct per_vhost_data__minimal *vhd) {

	uint32_t oldest_tail = lws_ring_get_oldest_tail(vhd->ring);
	struct per_session_data__minimal *old_pss = NULL;
	int most = 0, before = (int)lws_ring_get_count_waiting_elements(vhd->ring,
					&oldest_tail), m;

	/*
	 * At least one guy with the oldest tail has lagged too far, filling
	 * the ringbuffer with stuff waiting for them, while new stuff is
	 * coming in, and they must close, freeing up ringbuffer entries.
	 */

	lws_start_foreach_llp_safe(struct per_session_data__minimal **,
			      ppss, vhd->pss_list, pss_list) {

		if ((*ppss)->tail == oldest_tail) {
			old_pss = *ppss;

			lwsl_user("Killing lagging client %p\n", (*ppss)->wsi);

			lws_set_timeout((*ppss)->wsi, PENDING_TIMEOUT_LAGGING,
					/*
					 * we may kill the wsi we came in on,
					 * so the actual close is deferred
					 */
					LWS_TO_KILL_ASYNC);

			/*
			 * We might try to write something before we get a
			 * chance to close.  But this pss is now detached
			 * from the ring buffer.  Mark this pss as culled so we
			 * don't try to do anything more with it.
			 */

			(*ppss)->culled = 1;

			/*
			 * Because we can't kill it synchronously, but we
			 * know it's closing momentarily and don't want its
			 * participation any more, remove its pss from the
			 * vhd pss list early.  (This is safe to repeat
			 * uselessly later in the close flow).
			 *
			 * Notice this changes *ppss!
			 */

			lws_ll_fwd_remove(struct per_session_data__minimal,
					  pss_list, (*ppss), vhd->pss_list);

			/* use the changed *ppss so we won't skip anything */

			continue;

		} else {
			/*
			 * so this guy is a survivor of the cull.  Let's track
			 * what is the largest number of pending ring elements
			 * for any survivor.
			 */
			m = (int)lws_ring_get_count_waiting_elements(vhd->ring,
							&((*ppss)->tail));
			if (m > most)
				most = m;
		}

	} lws_end_foreach_llp_safe(ppss);

	/* it would mean we lost track of oldest... but Coverity insists */
	if (!old_pss)
		return;

	/*
	 * Let's recover (ie, free up) all the ring slots between the
	 * original oldest's last one and the "worst" survivor.
	 */

	lws_ring_consume_and_update_oldest_tail(vhd->ring,
		struct per_session_data__minimal, &old_pss->tail, before - most,
		vhd->pss_list, tail, pss_list);

	lwsl_user("%s: shrunk ring from %d to %d\n", __func__, before, most);
}