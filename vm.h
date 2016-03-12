#ifndef _VM_H_
#define _VM_H_

#include <stdio.h>
#include <pthread.h>
#include "message.h"


typedef int bool;
#define true 1
#define false 0

#define MAX_TICKS_PER_SECOND 6
#define MAX_ACTION_CHOICES 15
#define NUM_VMS 3
#define LOG_EXTENSION ".out"
#define BUF_MSG_COUNT 50

struct vm_args {
	int id;
	int *all_ids;
	int sec_to_run;
};

struct vm {
	int id;

	int srv_sock;
	int cli_sock[NUM_VMS - 1];
	/* IDs of VMs corresponding to cli_sock above */
	int cli_ids[NUM_VMS - 1]; 

	FILE *logfile;

	/* time (s) between instructions/ticks */
	float sleep_time;

	int lc;

	/* number of ticks passed */
	int ticks;
	int end_tick;

	/* fifo linked list of messages */
	struct message *msg_head; // newest
	struct message *msg_tail; // oldest
	int msg_count;
	/* protects queue & its count */
	pthread_mutex_t msg_lock;
};

void vm_main(struct vm_args *args);
void test_msglist_main();

#endif /* _VM_H_ */

