#ifndef _VM_H_
#define _VM_H_

#include <stdio.h>
#include <pthread.h>
#include "message.h"


typedef int bool;
#define true 1
#define false 0

/* Runtime parameters */
#define MAX_TICKS_PER_SECOND 6      // maximum number of instructions that a VM 
                                    // may execute per second; each VM chooses
                                    // a limit randomly between 1 and this max
#define MAX_ACTION_CHOICES 15       // number of types of actions that can be 
                                    // simulated per cycle
#define NUM_VMS 3                   // number of VMs in the simulation
#define LOG_EXTENSION ".out"        // extension for logfiles

/* 
 * This file defines `struct vm`, our representation of a VM. `vm.h` exposes
 * only the main run routine `vm_main` for a single VM. This handles 
 * initialization of sockets, background thread, and other resources, and
 * then enters the main instruction loop. It exits after `sec_to_run` seconds
 * (specified at initialization) have elapsed.
 *
 * Note: `vm.c` contains all vm-related helper functions (e.g. initialization, 
 * pulling and pushing from message list, logging, etc.) but the main loop is 
 * self-sufficient, and developers generally have no business calling the 
 * functions in vm.c directly.
 */

struct vm {
    /* This VM's ID. Must be unique from the other VMs for the simulation
     * to function properly. */
    int id;

    /* Serving socket, where this VM receives all messages from other VMs */
    int srv_sock;
    /* Client connections to other VM's server sockets; this VM sends messages 
     * to other clients via these sockets */
    int cli_sock[NUM_VMS - 1];
    /* IDs of VMs corresponding to cli_sock above */
    int cli_ids[NUM_VMS - 1]; 

    /* This VM's log file, named [id].out */
    FILE *logfile;

    /* This VM's logical clock */
    int lc;

    /* A 'tick' equals one tick of this VM's simulated system clock; that is, 
     * one instruction can execute per tick. `ticks` is the VM's tick-counter, and
     * the system exits after `end_tick` ticks have elapsed. After each 
     * instruction, we increment `ticks`.
     * 
     * So, if `sec_to_run` is 10, and we want to operate at 2 instructions per 
     * second, then `end_tick` would be 20.
     */
    int ticks;
    int end_tick;

    /* Time (in seconds) that this VM sleeps between instructions/ticks 
     * to maintain its simulated clock rate */
    float sleep_time;

    /* VM's message queue, a FIFO linked list of messages. */
    struct message *msg_head; // newest
    struct message *msg_tail; // oldest
    int msg_count;
    /* Protects access of queue & its count between background message reading daemon 
     * and main VM thread */
    pthread_mutex_t msg_lock;
};

/* 
 * Arguments `struct vm` needs for initialization.
 */
struct vm_args {
    /* This VM's ID. */
    int id;
    /* IDs of all VMs in the system. */
    int *all_ids; 
    /* Seconds the simulation should run. */
    int sec_to_run;
};

/* 
 * Main routine for a VM to run. This initializes the VM's sockets, background 
 * queue-handler thread, and other resources, and then enters the main 
 * instruction loop. It exits after `sec_to_run` seconds (specified at 
 * initialization) have elapsed.
 */ 
void vm_main(struct vm_args *args);

#endif /* _VM_H_ */

