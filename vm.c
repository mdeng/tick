#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 

/* generate random int from [0, max) */
// TODO: this isn't ~perfectly~ random
static int randint(int max) {
	return rand() % max;
}

/* id must be less than 100 */
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
	char log_name[strlen(LOG_EXTENSION) + 3 /* null + id length */ ];
	result = sprintf(log_name, "%d%s", id, LOG_EXTENSION);
	if (result < 0) {
		goto err_file;
	}

	printf("%s\n", log_name);
	vm->logfile = fopen((const char *)log_name, "w+");
	if (vm->logfile == NULL) {
		goto err_file;
	}

	/* create sockets */
	vm->srv_sock = socket(AF_UNIX, SOCK_STREAM, 0); 
	if (vm->srv_sock == -1) {
		goto err_srv_sock;
	}
	for (int i = 0; i < NUM_VMS - 1; i++) {
		vm->cli_sock[i] = socket(AF_UNIX, SOCK_STREAM, 0);
		if (vm->cli_sock[i] == -1) {
			goto err_cli_socks;
		}
	}

	result = pthread_mutex_init(&vm->msg_lock, NULL);
	if (result) {
		goto err_cli_socks;
	}

	vm->lclock = 0;
	vm->ticks = 0;

	int ticks_per_s = randint(MAX_TICKS_PER_SECOND);
	vm->sleep_time = 1.0 / ticks_per_s;
	vm->end_tick = SECONDS_TO_RUN * ticks_per_s;

	vm->msgs = NULL;

	return vm;

err_cli_socks:	
	for (int i = 0; i < NUM_VMS - 1; i++) {
		if (vm->cli_sock[i] < 0) {
			close(vm->cli_sock[i]);
		}
	}
	if (vm->srv_sock < 0) {
		close(vm->srv_sock);
	}
err_srv_sock:
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
