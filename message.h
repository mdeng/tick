#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "vm.h"

/* 
 * This file defines `struct message`, which is our internal representation 
 * of messages passed from one VM to another; it is `struct message` that
 * is queued in `struct vm` for processing.
 * 
 * Additionally, this file exposes routines for safely creating and freeing 
 * messages, and for sending and reading messages over a socket.
 */

/*   
 * Each socket message consists of 3 int32s, for a total length of 12 bytes:
 * 	checksum (4 bytes)
 * 	sender id (4 bytes)
 * 	sender logical clock (4 bytes)
 * 
 * We keep the ID and logical clock in `struct message`.
 */

#define MSG_LEN 12		   // number of bytes that the message takes

struct message {
	/* messages are queued in struct VM as linked lists.*/
	struct message *prev;	
	struct message *next;

	/* ID of the sender VM */
	int sender_id;

	/* Logical clock of the sender, at the time the message was sent */
	int sender_lc;
};

/* Safely create a `struct message`. Returns NULL on error. */
struct message *msg_create(int sender_id, int sender_lc);
/* Safely destroy (free all resources) of a `struct message`. */
void msg_destroy(struct message *msg);

/* Send a message to the socket represented by `sockfd`, from the VM with
 * id `sender_id` and logical clock value `sender_lc`. */
void msg_send(int sockfd, int sender_id, int sender_lc);

/* Read a message from the socket represented by `sockfd`, using buffer `buf` 
 * of length `buflen`. Return message formatted as a `struct message`, or NULL
 * on error. */
struct message *msg_read(int sockfd, int *buf, int buflen);

/* Print out contents of a message. Debugging utility. */
void msg_print(struct message *msg);

#endif /* MESSAGE_H_ */