
# Overview

This package models a small, asynchronous distributed system consisting of 3 
machines running at different speeds. In the simulated system, each machine 
maintains a logical clock, and they pass messages about their logical clock 
values to each other over a network and log events. The 3 VMs will be given 
the IDs 1, 2, and 3, and they will respectively log to the files `1.out`, 
`2.out`, and `3.out`.

A fuller description of their behavior is in `specs.txt`.  


# Running 

* to build: `make`
* to run: `./lclock [seconds_to_run]`
* to clean: `make clean`

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

Further adjustments to settings (e.g. number of VMs to spawn, extension of
logfiles, etc.) can be adjusted in `vm.h`.


# System Architecture 

We fork 3 processes, each of which simulates a VM, encapsulated by `struct vm`
defined in `vm.c|h`. The VMs communicate to each other over UNIX sockets. Specifically, each VM is assigned a unique ID and knows the IDs of the others. 
During initialization, a VM will open a server socket named `[id].sock` and 
connect as clients to the sockets of the other VMs.

As per the specs, each VM's run loop is constrained to execute at some randomly
chosen `n` between 1 and 6 instructions per second, where an instruction is 
defined as one of the following:
* Writing to the log
* Incrementing the logical clock
* Reading a message from its message queue
* Sending a message to other clients.

To do this, we have one function per instruction, and each of these functions
sleeps for `1/n` seconds (as defined by the system clock) before returning.

To allow incoming messages on the socket to be processed at a speed independent 
of the simulated clock rate, we separate queued network messages from an internal message queue, maintained on the `struct vm` itself. The VM's main run loop only 
ever touches this internal queue, and can only pop one every `1/n` s. 

Meanwhile, during initialization we have each VM fork a background thread, whose  task is to pull (bytestream) messages off its server socket and add it to the VM's internal representation of a message queue. This background thread is mostly unconstrained in speed, except we synchronize access of the message queue 
between the main and background threads using a mutex. Note that messages are 
only sent in the client -> server direction over these sockets.

After running for `seconds_to_run` (default 60) seconds, which defaults to 60 seconds, each VM simulation process will independently call `exit()`. The parent 
launcher process will terminate after all these children processes have 
exited.



# Files

* `message.c/h`: Defines `struct message`, which represents messages passed from 
one VM to another, containing information about the sender's identity and 
logical clock value at time of sending. Also contains utilities for sending and 
receiving messages from a socket.

* `vm.c/h`: Defines `struct vm`, which represents a simulated VM. Each VM should 
run in its own process in the same working directory, and each needs to have 
unique IDs. `vm.h` merely exposes the main run loop for a single VM; `vm.c` 
contains more helpers that the VM needs to function (e.g. initialization, 
pulling and pushing from message list, etc.).

* `main.c`: Startup routine that forks 3 processes and launches a VM simulation 
in each, under the IDs 1, 2, and 3. The start routine will wait for all the VMs 
to exit before itself exiting.

* `Makefile`: Used to build and clean the package.

* `README.md`: This file!