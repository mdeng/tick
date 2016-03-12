#include "vm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h> 


/////////////////
// UTILITIES

typedef int bool;
#define true 1
#define false 0

#define max(a, b) (((a) > (b)) ? (a) : (b)) 


/* generate random int from [0, bound) */
static int randint(int bound, int id) {
	struct timeval now;
	gettimeofday(&now, NULL);
	srand((now.tv_usec % 10000) * (id+1));

	return rand() % bound;
}


/////////////////
// VM

struct vm *vm_create(int id) {
	int result;
	struct vm *vm;

	vm = malloc(sizeof(*vm));
	if (vm == NULL) {
		return NULL;
	}

	vm->id = id;

	/* open logfile, named [id].out */
	char log_name[strlen(LOG_EXTENSION) + 10 /* enoughf or null + id length */ ];
	result = sprintf(log_name, "%d%s", id, LOG_EXTENSION);
	if (result < 0) {
		goto err_file;
	}

	vm->logfile = fopen((const char *)log_name, "w+");
	if (vm->logfile == NULL) {
		goto err_file;
	}

	result = pthread_mutex_init(&vm->msg_lock, NULL);
	if (result) {
		goto err_mutex;
	}

	/* init sockets to invalid FD */
	vm->srv_sock = -1;
	for (int i = 0; i < NUM_VMS - 1; i++) {
		vm->cli_sock[i] = -1;
	}
	/* init client IDs to invalid */
	for (int i = 0; i < NUM_VMS - 1; i++) {
		vm->cli_ids[i] = -1;
	}
	vm->lc = 0;
	vm->ticks = 0;

	int ticks_per_s = randint(MAX_TICKS_PER_SECOND, id) + 1;
	printf("ticks_per_s: %d\n", ticks_per_s);
	vm->sleep_time = 1.0 / ticks_per_s;
	vm->end_tick = SECONDS_TO_RUN * ticks_per_s;
	printf("end tick: %d\n", vm->end_tick);

	vm->msg_head = NULL;
	vm->msg_tail = NULL;
	vm->msg_count = 0;

	return vm;

err_mutex:
	fclose(vm->logfile);
err_file:
	free(vm);
	return NULL;
}

void vm_destroy(struct vm *vm) {
	pthread_mutex_destroy(&vm->msg_lock);

	for (int i = 0; i < NUM_VMS - 1; i++) {
		if (vm->cli_sock[i] != -1) {
			close(vm->cli_sock[i]);
		}
	}
	if (vm->srv_sock != -1) {
		close(vm->srv_sock);
	}

	fclose(vm->logfile);
	free(vm);
}

void vm_inc_ticks(struct vm *vm) {
	vm->ticks += 1;
	usleep(1000000 * vm->sleep_time);

	/* time's up; clean up resources and exit */
	if (vm->ticks == vm->end_tick) {
		vm_destroy(vm);
		exit(0); /* this will terminate the daemon too */
	}
}

void vm_push_message(struct vm *vm, struct message *new_msg) {
	if (new_msg == NULL) {
		return;
	}

	pthread_mutex_lock(&vm->msg_lock);

	if (vm->msg_count == 0) {
		assert(vm->msg_head == NULL);
		assert(vm->msg_tail == NULL);

		vm->msg_head = new_msg;
		vm->msg_tail = new_msg;
	} else {
		new_msg->next = vm->msg_head;
		new_msg->prev = NULL;
		new_msg->next->prev = new_msg;
		vm->msg_head = new_msg;
	}
	vm->msg_count++;

	pthread_mutex_unlock(&vm->msg_lock);
}

struct message *vm_pop_message(struct vm *vm, time_t *rawtime) {
	struct message *msg;

	pthread_mutex_lock(&vm->msg_lock);

	if (vm->msg_count == 0) {
		pthread_mutex_unlock(&vm->msg_lock);
		return NULL;
	}

	msg = vm->msg_tail;
	if (vm->msg_count == 1) {
		vm->msg_head = NULL;
		vm->msg_tail = NULL;
	} else {
		vm->msg_tail = msg->prev;
		vm->msg_tail->next = NULL;
	}
	vm->msg_count--;

	pthread_mutex_unlock(&vm->msg_lock);

	time(rawtime);
	vm_inc_ticks(vm);
	return msg;
}

// TODO error handling
int init_srv_sock(int *sock_fd, const char *sock_name) {
	int len;
	struct sockaddr_un sa;

	*sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*sock_fd < 0) {
		return -1;
	}

	unlink(sock_name);
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, sock_name);
	len = strlen(sa.sun_path) + sizeof(sa.sun_family) + 1;
	if (bind(*sock_fd, (struct sockaddr *)&sa, len) < -1) {
		perror("bind");
		return -1;
	}
	if (listen(*sock_fd, 5 /* queue_limit */) < -1) {
		perror ("listen");
		return -1;
	}
	return 0;
}

// TODO actual error handling
int init_cli_sock(int *sock_fd, const char *remote_name, int vm_id) {
	int len;
	struct sockaddr_un sa;

	bool done = false;

	while (!done) {
		*sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (*sock_fd < 0) {
			continue;
		}

		sa.sun_family = AF_UNIX;
	    strcpy(sa.sun_path, remote_name);
	    len = strlen(sa.sun_path) + sizeof(sa.sun_family) + 1;

	    if (connect(*sock_fd, (struct sockaddr *)&sa, len) == -1) {
	        /* This commonly errors out if the other server hasn't initalized 
	         * the socket yet. */
	        close(*sock_fd);
	        continue;
	    }
	    done = true;
	}
    return 0;
}

void vm_init_srv_sockets(struct vm *vm) {
	char name[10]; /* [id].sock */

	sprintf(name, "%d.sock", vm->id);
	if (init_srv_sock(&vm->srv_sock, name) < 0) {
		perror("could not init srv sock");
		exit(1);
	}
}

void vm_init_cli_sockets(struct vm *vm, struct vm_args *args) {
	char name[10]; /* [id].sock */
	int sock_idx = 0;
	for (int i = 0; i < NUM_VMS; i++) {
		if (args->all_ids[i] != vm->id) { 
			vm->cli_ids[sock_idx] = args->all_ids[i];
			sprintf(name, "%d.sock", args->all_ids[i]);
			init_cli_sock(&vm->cli_sock[sock_idx], name, vm->id);
			sock_idx++;
		}
	}
}

/* for reading messages from the socket */
#define READBUFLEN 12
int read_buf[3];

/* Will have two of these, one for handling each client (other VM) */
void *vm_message_daemon(void *args) {
	struct vm *vm = (struct vm *)args;

	int conn_fd;
	struct sockaddr_un remote_sa;

	socklen_t len = sizeof(remote_sa);
	conn_fd = accept(vm->srv_sock, (struct sockaddr *)&remote_sa, &len);

	while (true) {
		struct message *new_msg;

		/* get a message off the socket, and push it on the queue*/
		new_msg = msg_read(conn_fd, read_buf, READBUFLEN);
		vm_push_message(vm, new_msg);
	}
}

void vm_update_lc(struct vm *vm, int other_lc) {
	assert(vm != NULL);
	vm->lc = max(vm->lc, other_lc) + 1;
	vm_inc_ticks(vm);
}

void vm_log_receive(struct vm *vm, struct message *msg, time_t rawtime) {
	/* write that it received a message, the global system time, the length 
	 * of the message queue, and the logical clock time. */
	char buf[128];
	size_t len, written;
	struct tm *timeinfo;

  	timeinfo = localtime(&rawtime);

	len = sprintf(buf, "%s[RECEIVE] sender ID: %d | sender LC: %d | local LC: %d | msg_count %d\n\n", 
		asctime(timeinfo), msg->sender_id, msg->sender_lc, vm->lc, vm->msg_count);
	written = fwrite(buf, sizeof(char), len, vm->logfile);
	if (written != len) {
		perror("log write");
	}

	vm_inc_ticks(vm);
}

// TODO idx change to sockname or target id
void vm_log_send(struct vm *vm, int dest_idx, time_t rawtime) {
	char buf[128];
	size_t len, written;
	struct tm *timeinfo;

  	timeinfo = localtime(&rawtime);

  	if (dest_idx == 2) {
  		len = sprintf(buf, "%s[SEND BOTH] local LC: %d\n\n", 
			asctime(timeinfo), vm->lc);
  	} else {
		len = sprintf(buf, "%s[SEND ONE] destination id: %d | local LC: %d\n\n", 
			asctime(timeinfo), vm->cli_ids[dest_idx], vm->lc);
	}
	written = fwrite(buf, sizeof(char), len, vm->logfile);
	if (written != len) {
		perror("log write");
	}

	vm_inc_ticks(vm);
}

void vm_log_internal(struct vm *vm, time_t rawtime) {
	char buf[128];
	size_t len, written;
	struct tm *timeinfo;

  	timeinfo = localtime(&rawtime);

	len = sprintf(buf, "%s[INTERNAL] local LC: %d\n\n", 
		asctime(timeinfo), vm->lc);
	written = fwrite(buf, sizeof(char), len, vm->logfile);
	if (written != len) {
		perror("log write");
	}

	vm_inc_ticks(vm);
}

int vm_generate_action_type(struct vm *vm) {
	int action_type;

	action_type = randint(MAX_ACTION_CHOICES, vm->id);
	vm_inc_ticks(vm);
	return action_type;
}

void vm_send_message(struct vm *vm, int sock_idx, time_t *rawtime) {
	msg_send(vm->cli_sock[sock_idx], vm->id, vm->lc);
	time(rawtime);
	vm_inc_ticks(vm);
}

void vm_run_cycle(struct vm *vm) {
	time_t rawtime;

	/* check for messages */
	struct message *msg = vm_pop_message(vm, &rawtime);
	if (msg != NULL) {
		vm_update_lc(vm, msg->sender_lc);
		vm_log_receive(vm, msg, rawtime);
		msg_destroy(msg);
	} else {
		int action_type = vm_generate_action_type(vm);
		printf("Action type: %d, ID: %d\n", action_type, vm->id);
		switch(action_type) {
			case 0:
				vm_send_message(vm, 0, &rawtime);
				vm_update_lc(vm, 0 /* other_lc */);
				vm_log_send(vm, 0, rawtime);
				break;
			case 1:
				vm_send_message(vm, 1, &rawtime);
				vm_update_lc(vm, 0 /* other_lc */);
				vm_log_send(vm, 1, rawtime);
				break;
			case 2:
				vm_send_message(vm, 0, &rawtime); 
				vm_send_message(vm, 1, &rawtime); // 2nd time will be recorded
				vm_update_lc(vm, 0 /* other_lc */);
				vm_log_send(vm, 2, rawtime);
				break;
			default:
				assert(action_type < MAX_ACTION_CHOICES);
				vm_update_lc(vm, 0 /* other_lc */);
				vm_log_internal(vm, rawtime);
				break;
		}
	}
}

void vm_main(struct vm_args *args) {
	struct vm *vm;

	vm = vm_create(args->id);
	if (vm == NULL) {
		return;
	}
	vm_init_srv_sockets(vm);

	pthread_t threads[NUM_VMS - 1];
	for (int i = 0; i < NUM_VMS - 1; i++) {
		if (pthread_create(&threads[i], NULL, &vm_message_daemon, (void *)vm)) {
			perror("create");
		}
	}

	vm_init_cli_sockets(vm, args);

	while(true) {
		vm_run_cycle(vm);
	}
}

// TODO: logging functions n stuff

void vm_print_messages(struct vm *vm) {
	printf("messages: \n");
	struct message *cur = vm->msg_head;
	while (cur != NULL) {
		msg_print(cur);
		cur = cur->next;
	}
}

/* print VM fields; for debugging */
void vm_print(struct vm *vm) {
	printf("[VM %d]: lc %d | sleep_time %f | ticks %d\n", 
		vm->id, vm->lc, vm->sleep_time, vm->ticks);
	vm_print_messages(vm);
}
