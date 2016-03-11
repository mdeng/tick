#!/usr/bin/python

import sys, socket, random, time, datetime
import multiprocessing as mp
import Queue


TIME_TO_RUN_SECONDS = 60
PORTS = [8080, 8081, 8082]

class VM:
    def __init__(self, name, server_sock, client_sock, server_conn):
        self.name = name
        self.ss = server_sock
        self.cs = client_sock
        self.sc = server_conn

        self.logfile = open(name + '.out', 'w')

        # rand clock rate
        self.num_actions = random.randint(1,6)
        self.sleep_time = 1./num_actions

        # initialize LC
        self.lc = 0

        # initialize message queue
        self.msg_q = Queue.Queue()

        # i
        self.ticks = 0
        self.end_time = num_actions * TIME_TO_RUN_SECONDS

    def shutdown(self):
        self.ss.close()
        self.cs.close()
        self.logfile.close()
        sys.exit()

    def inc_ticks(self):
        time.sleep(sleep_time)
        self.ticks += 1
        if self.ticks == self.end_time:
            self.shutdown()

    ###### Tick-sized actions

    def inc_lc(self):
        self.lc += 1
        self.inc_ticks()

    def sync_lc(self, other_lc):
        self.lc = max(self.lc + 1, other_lc)
        self.inc_ticks()

    def generate_action_type(self):
        action_type = random.randint(0, 9)
        self.inc_ticks() # TODO: remove this?
        return action_type

    def log_send(self):
        # TODO: add process name (or port)
        self.logfile.write( \
            "[Sent message] \n" \
            "Logical clock: %d \n "\
            "System time: %s\n" \
            % (self.lc, datetime.datetime.now()))
        self.inc_ticks()

    def log_recv(self, other_name):
        self.logfile.write( \
            "[Received message] \n" \
            "Sender: %s" \
            "Num messages queued: %d" \
            "Logical clock: %d \n "\
            "System time: %s\n" \
            % (other_name, len(self.msg_q), self.lc, datetime.datetime.now()))
        self.inc_ticks()

    def log_internal(self):
        self.logfile.write( \
            "[Internal event] \n" \
            "Logical clock: %d \n "\
            "System time: %s\n" \
            % (self.lc, datetime.datetime.now()))
        self.inc_ticks()


    ######

    def pop_msg(self, sock):
        # TODO 
        # return msg, vm name
        return None

    def process_send(self, sock):
        self.send_message(sock)
        self.inc_lc()
        self.log_send()

    def process_msg(msg):
        (other_lc, other_name) = msg
        self.sync_lc(other_lc)
        self.log_recv()

    #####

    def run_cycle(self):
        # TODO: vary order of checking conn0/1

        # check for messages
        msg = self.pop_msg(self.sc)
        if msg:
            self.process_msg(msg)
            return
        
        # check second socket for messages
        # i don't know if we actually need this
        #msg, vm_name = self.receive_message(self.sc)
        #if msg:
        #    self.process_msg(msg, vm_name)
        #    return

        action_type = self.generate_action_type()
        
        if (action_type == 0):
            self.process_send(self.sc)

        elif (action_type == 1):
            self.process_send(self.cs)

        elif (action_type == 2):
            self.process_send(self.sc)
            self.process_send(self.cs)

        else:
            self.inc_lc()
            self.log_internal()

    def run(self):
        while self.ticks < self.end_time:
            self.run_cycle()


#### MAIN

def init_server_socket(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((socket.gethostname(), port))
    sock.listen(5)
    return sock

def connect_client_socket(port):
    while True:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        result = sock.connect_ex((socket.gethostname(), port))
        if result:
            print result
            sock.close()
        else:
            return sock

def vm_main(name, ports):
    ss = init_server_socket(ports[0]) 
    cs = connect_client_socket(ports[1])   
    sc, _ = s.accept() # will block until other VM connects

    vm = VM(name, ss, cs, sc)
    vm.run()

    #c.send(name)
    #str = ss.recv(4096)
    #print str

    #s.close()
    #c.close()

def launch_vm(name, ports):
    vm = mp.Process(target=vm_main, args=(name, ports))
    vm.start()
    return vm

if __name__ == '__main__':
    vm1 = launch_vm("vm1", (PORTS[0], PORTS[1]))
    vm2 = launch_vm("vm2", (PORTS[1], PORTS[2]))
    vm3 = launch_vm("vm3", (PORTS[2], PORTS[0]))

    vm1.join()
    vm2.join()
    vm3.join()
    