#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h> 

#define BUFLEN 1024

/* generate random int from [0, max) */
// TODO: this isn't ~perfectly~ random
static int randint(int max) {
	return rand() % max;
}

/////////////////
// VM

struct vm *vm_create(int id) {
	int result;
	struct vm *vm;

	if (id > MAX_ID) {
		return NULL;
	}

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

	vm->lclock = 0;
	vm->ticks = 0;

	int ticks_per_s = randint(MAX_TICKS_PER_SECOND);
	vm->sleep_time = 1.0 / ticks_per_s;
	vm->end_tick = SECONDS_TO_RUN * ticks_per_s;

	vm->msgs = NULL;
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
	        perror("connect");
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
			sprintf(name, "%d.sock", args->all_ids[i]);
			init_cli_sock(&vm->cli_sock[sock_idx], name, vm->id);
			sock_idx++;
		}
	}
}

/* Will have two of these, one for handling each client (other VM) */
void *vm_message_daemon(void *args) {
	struct vm *vm = (struct vm *)args;

	int conn_fd;
	struct sockaddr_un remote_sa;

	char buf[BUFLEN];

	socklen_t len = sizeof(remote_sa);
	conn_fd = accept(vm->srv_sock, (struct sockaddr *)&remote_sa, &len);
/*
	while (len = recv(conn_fd, &buf, BUFLEN, 0), len > 0) {
	    // parse the message in buf TODO
	    break;
	}*/
	pthread_exit(NULL);
}

int vm_main(struct vm_args *args) {
	struct vm *vm;

	vm = vm_create(args->id);
	if (vm == NULL) {
		return 1;
	}
	vm_init_srv_sockets(vm);

	pthread_t threads[NUM_VMS - 1];
	for (int i = 0; i < NUM_VMS - 1; i++) {
		if (pthread_create(&threads[i], NULL, &vm_message_daemon, (void *)vm)) {
			perror("create");
		}
	}

	vm_init_cli_sockets(vm, args);

	/// DO STUFF IN A LOOP

	for (int i = 0; i < NUM_VMS - 1; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}

struct message *vm_read_queued(struct vm *vm) {
	return msg_read(vm->srv_sock, vm->read_buf, sizeof(vm->read_buf));
}

// TODO: logging functions n stuff

void vm_print(struct vm *vm) {
	printf("[VM %d]: lclock %d | sleep_time %f | ticks %d\n", 
		vm->id, vm->lclock, vm->sleep_time, vm->ticks);
}


// TODO DELETE THIS - it's just an example to test sending
int test_msg_main(int id, int all_ids[]) {
	printf("vm %d in main!!!\n", id);

	int sockfd;

	if (id == 1) {
		init_srv_sock(&sockfd, "test.sock");
		int x;
		struct sockaddr_un remote_sa;


		socklen_t len = sizeof(remote_sa);

		if ((x = accept(sockfd, (struct sockaddr *)&remote_sa, &len)) && x > 0) {
			printf("got connection\n");
			int readbuf[3];
			struct message *msg = msg_read(x, readbuf, sizeof(readbuf));
			msg_print(msg);
		} else {
			perror("accept");
		}
	} else if (id == 2) {
		sleep(1);
		int res = init_cli_sock(&sockfd, "test.sock", id);
		sleep(1);
		if (res == 0) {
			printf("sending...\n");
			msg_send(sockfd, 1, 2);
		} else {
			perror("connect");
			exit(-1);
		}
	}

	return 0;
}
