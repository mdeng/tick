#include "message.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h> 

/////////////////
// MESSAGES

struct message *msg_create(sender_id, sender_lc) {
	int result;
	struct message *msg;

	msg = malloc(sizeof(*msg));
	if (msg == NULL) {
		return NULL;
	}

	msg->sender_id = sender_id;
	msg->sender_lc = sender_lc;

	msg->prev = NULL;
	msg->next = NULL;
	return msg;
}

void msg_destroy(struct message *msg) {
	free(msg);
}

void msg_print(struct message *msg) {
	assert(msg != NULL);
	printf("[MESSAGE] sender_id %d | sender_lc %d\n", msg->sender_id, msg->sender_lc);
}

void msg_send(int sockfd, int sender_id, int lc) {
	/* 
	 * a message is 3 int32s:
	 * checksum (4 bytes)
	 * sender id (4 bytes)
	 * sender lc (4 bytes)
	 */
	int buf[3];
	buf[0] = MSG_CHECKSUM;
	buf[1] = sender_id;
	buf[2] = lc;

	ssize_t sent = send(sockfd, buf, MSG_LEN, 0 /* flags */);
	if (sent != MSG_LEN) {
		printf("vm %d: failure to send @ lc %d\n", sender_id, lc);
	}
}

/* Reads one message at a time from the socket. Assumes everything is dandy. */
struct message *msg_read(int sockfd, int *buf, int buflen) {
	ssize_t received = recv(sockfd, buf, buflen, 0);

	if (received < buflen) {
		return NULL;
	}

	if (buf[0] != MSG_CHECKSUM) {
		printf("read fail: checksum mismatch\n");
		return NULL;
	}

	struct message *msg = msg_create(buf[1] /* id */, buf[2] /*lc */);
	return msg; /* null if allocation error */
}
