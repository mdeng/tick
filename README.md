# Basics

* to build: `make`
* to run: `./lclock [seconds_to_run]`
* to clean: `make clean`

By default, VMs will be given the IDs 1, 2, and 3. 
These will respectively log to the files 1.out, 2.out, and 3.out.

Example:
```
user@machine$ ./lclock 10
Running simulation for 10 seconds.
VM 0 speed: 4 ticks/s
VM 1 speed: 5 ticks/s
VM 2 speed: 4 ticks/s
.........................................
Done!
```

# Files

* message.c/h: Defines `struct message`, which represents messages passed from one VM to another, containing information about the sender's identity and logical clock value at time of sending.

* vm.c/h: Defines `struct vm`, which represents a simulated VM. Each VM should run in its own process in the same working directory, and each needs to have unique IDs. vm.h merely exposes the main run loop for a single VM; vm.c contains all the helpers that the VM needs to function (e.g. initialization helpers, pulling and pushing from message list, etc.).

* main.c: Startup routine that forks 3 processes and launches a VM simulation in each, under the IDs 1, 2, and 3. The start routine will wait for all the VMs to exit before itself exiting.

* Makefile: Used to build and clean the package.

* README.md: This file!