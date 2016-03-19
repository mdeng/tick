#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_SEC_TO_RUN 60


/*
 * Main routine.
 * Forks 3 processes, each of which simulates a VM by running
 * the main loop of `struct vm`. 
 * The VM simulations will exit after running for `num_seconds_to_run`
 * seconds, as defined by the system clock. Then, the parent will
 * exit and output `Done.`
 */
int main(int argc, char *argv[]) {
	/* configure number of seconds to run each VM */
	int sec_to_run;
	if (argc == 1) {
		sec_to_run = DEFAULT_SEC_TO_RUN;
	} else if (argc == 2) {
		sec_to_run = atoi(argv[1]);
	} else {
		printf("Usage: ./lclock [seconds_to_run]\n");
		return 1;
	}
	printf("Running simulation for %d seconds.\n", sec_to_run);

	/* save child IDs so that we can wait for children to exit */
	pid_t vm_pids[NUM_VMS];

	int ids[NUM_VMS];
	for (int i = 0; i < NUM_VMS; i++) {
		ids[i] = i;
	}

	for (int i = 0; i < NUM_VMS; i++) {
		ids[i] = i;
		pid_t pid = fork();

		if (pid == 0) { /* child */
			struct vm_args args;
			args.id = i;
			args.all_ids = ids;
			args.sec_to_run = sec_to_run;
			vm_main(&args); 
			return 0; /* unreachable — child should call exit() */
		} else { /* parent */
			vm_pids[i] = pid; /* parent */
		}
	}

	/* parent should wait for children to exit */
	for (int i = 0; i < NUM_VMS; i++) {
		waitpid(vm_pids[i], NULL, 0);
	}
	printf("\nDone!\n");
	return 0;
}