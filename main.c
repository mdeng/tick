#include "vm.h"

#include <stdio.h>

int main() {
	for (int i = 0; i < NUM_VMS; i++) {
		// do something
	}
	struct vm *vm = vm_create(1);
	vm_destroy(vm);
	printf("fuck this\n");
}