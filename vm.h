#ifndef _VM_H_
#define _VM_H_

#include <stdio.h>
#include <pthread.h>
#include "message.h"


typedef int bool;
#define true 1
#define false 0

#define MAX_TICKS_PER_SECOND 6
#define SECONDS_TO_RUN 60
#define NUM_VMS 3
#define LOG_EXTENSION ".out"
#define BUF_MSG_COUNT 50

struct vm_args {
	int id;
	int *all_ids;
};

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
	int msg_count;
	/* protects queue & its count */
	pthread_mutex_t msg_lock;

	/* for reading messages from the socket */
	int read_buf[3];

};

int vm_main();

#endif /* _VM_H_ */

