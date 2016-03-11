#include "vm.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
	/* allow parent to wait for children */
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
			vm_main(&args); 
			return 0; /* unreachable — child should call exit() */
		} else { /* parent */
			vm_pids[i] = pid; /* parent */
		}
	}

	/* parent should wait for children to exit */
	for (int i = 0; i < NUM_VMS; i++) {
		printf("waiting for VM %d (pid %d)\n", i, vm_pids[i]);
		waitpid(vm_pids[i], NULL, 0);
	}
	printf("done\n");
	return 0;
}