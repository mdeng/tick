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

// Background thread TODO
// Make sure this exits correctly...
// may have to change the prototype
int vm_message_daemon(struct vm *vm) {
	int conn_fd;
	struct sockaddr_un remote_sa;

	char buf[BUFLEN];

	socklen_t len = sizeof(remote_sa);
	conn_fd = accept(vm->srv_sock, (struct sockaddr *)&remote_sa, &len);

	while (len = recv(conn_fd, &buf, BUFLEN, 0), len > 0) {
	    // parse the message in buf TODO
	    break;
	}
	return 0;
	//pthread_exit()?
}

// TODO error handling
int vm_init_srv_sock(int *sock_fd, const char *sock_name) {
	int len;
	struct sockaddr_un sa;

	*sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*sock_fd < 0) {
		return -1;
	}

	unlink(sock_name);
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, sock_name);
	len = strlen(sa.sun_path) + sizeof(sa.sun_family);
	bind(*sock_fd, (struct sockaddr *)&sa, len);
	listen(*sock_fd, 5 /* queue_limit */);

	printf("init srv sock %s\n", sock_name);
	return 0;
}

// TODO actual error handling
int vm_init_cli_sock(int *sock_fd, const char *remote_name, int vm_id) {
	int len;
	struct sockaddr_un sa;

	*sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*sock_fd < 0) {
		return -1;
	}

	sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, remote_name);
    len = strlen(sa.sun_path) + sizeof(sa.sun_family);

    if (strcmp(remote_name, "0.sock")) {
    	printf("%d: abort connection — %s\n", vm_id, remote_name);
    	return -1;
    } 
    printf("%d: attempting to connect to %s\n", vm_id, remote_name);

    if (connect(*sock_fd, (struct sockaddr *)&sa, len) == -1) {
        perror("connect");
    }
    return 0;
}
	

void vm_init_sockets(struct vm *vm, int all_ids[]) {
	char name[10]; /* [id].sock */

	sprintf(name, "%d.sock", vm->id);
	vm_init_srv_sock(&vm->srv_sock, name);

	sleep(1);

	int sock_idx = 0;
	for (int i = 0; i < NUM_VMS; i++) {
		printf("VM %d init client\n", vm->id);
		if (all_ids[i] == vm->id) { 
			continue;
		}
		sprintf(name, "%d.sock", all_ids[i]);
		vm_init_cli_sock(&vm->cli_sock[sock_idx], name, vm->id);
		sock_idx++;
	}
}

int vm_main(int id, int all_ids[]) {
	struct vm *vm;

	vm = vm_create(id);
	if (vm == NULL) {
		return 1;
	}

	vm_init_sockets(vm, all_ids);

	// fork background thread
	// do the desired things in a loop

	return 0;
}

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

	printf("%s\n", log_name);
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
