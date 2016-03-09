#ifndef _VM_H_
#define _VM_H_

#include <stdio.h>
#include <pthread.h>

#define MAX_TICKS_PER_SECOND 6
#define SECONDS_TO_RUN 60
#define NUM_VMS 3
#define MAX_ID 100000 // PID_MAX
#define LOG_EXTENSION ".out"

struct message {
	struct message *prev;
	struct message *next;
	int sender_id;
	int sender_lc;
};

struct message *message_create(int sender_id, int sender_lc);

struct vm {
	int id;

	int srv_sock;
	int cli_sock[NUM_VMS - 1];

	FILE *logfile;

	/* time (s) between instructions/ticks */
	float sleep_time;

	int lclock;

	/* number of ticks passed */
	int ticks;
	int end_tick;

	/* fifo queue of messages */
	struct message *msgs;
	/* protects adding and removing from queue */
	pthread_mutex_t msg_lock;
};

/* alloc new vm */
struct vm *vm_create(int id);
/* free vm */
void vm_destroy(struct vm *vm);

/* initialize server socket */
int vm_init_srv(struct vm *vm);
/* initialize client sockets */
int vm_init_cli(struct vm *vm);

int vm_main();

int vm_message_daemon(struct vm *vm);

/* add new message to tail of queue */
int vm_push_message(struct message *msg);
/* fetch earliest message from queue */
struct message *vm_pop_message();


#endif /* _VM_H_ */

