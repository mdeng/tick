#include "vm.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>


typedef int bool;
#define true 1
#define false 0



int main() {
	/* allow parent to wait for children */
	pid_t vm_pids[NUM_VMS];
	int ids[NUM_VMS];

	for (int i = 0; i < NUM_VMS; i++) {
		ids[i] = i;
		pid_t pid = fork();

		if (pid == 0) { /* child */
			return vm_main(i, &ids);
		} else { /* parent */
			vm_pids[i] = pid; /* parent */
		}
	}

	/* parent should wait for children to exit */
	for (int i = 0; i < NUM_VMS; i++) {
		printf("waiting for %d\n", vm_pids[i]);
		waitpid(vm_pids[i], NULL, 0);
	}
	printf("done\n");
	return 0;
}