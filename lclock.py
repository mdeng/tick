import multiprocessing as mp
import logging
import socket
import time
import random
import math
import sys
import datetime
import Queue


TIME_TO_RUN_SECONDS = 60

def send_message(lc, sock):

# int - no message available is -1 
def receive_message(sock, queue):

def inc_time(i, f, sleep_time, end_time): 
    time.sleep(sleep_time)
    if (i + 1) == end_time:
        f.close()
        sys.exit()
    return i + 1

def process_rec(rec_message, proc_name, i, lc, sleep_time, \
                                 end_time, queue_len, f):
    lc = max(lc + 1, rec_message + 1)
    i = inc_time(i, f, sleep_time, end_time)
    global_time = str(datetime.datetime.now())
    f.write("Received message from process " + proc_name + \
            ". \nNum messages queued: " + \
            queue_len + "\nLogical clock: " + str(lc) + \
            "\nSystem time: " + global_time + "\n\n")
    i = inc_time(i, f, sleep_time, end_time)
    return (i, lc)

def process_send(conn, proc_name, i, lc, sleep_time, end_time, f):
    send_message(lc, sock)
    i = inc_time(i, f, sleep_time, end_time)
    lc += 1
    i = inc_time(i, f, sleep_time, end_time)
    return (i, lc)

def process_send_one(conn, proc_name, i, lc, sleep_time, end_time, f):
    (i, lc) = process_send(conn, proc_name, i, lc, sleep_time, end_time, f)
    f.write("Sent message to process " + proc_name + \
            ". \nLogical clock: " + str(lc) + \
            "\nSystem time: " + global_time + "\n\n")
    i = inc_time(i, f, sleep_time, end_time)
    return (i, lc)

def process_send_both(conns, proc_names, i, lc, sleep_time, end_time, f):
    (i, lc) = process_send(conns[0], proc_names[0], i, lc, \
                                        sleep_time, end_time, f)
    (i, lc) = process_send(conns[1], proc_names[1], i, lc, \
                                        sleep_time, end_time, f)
    f.write("Sent message to processes " + proc_names[0] + \
            " and " + proc_names[1] + \
            ". \nLogical clock: " + str(lc) + \
            "\nSystem time: " + global_time + "\n\n")
    i = inc_time(i, f, sleep_time, end_time)
    return (i, lc)

# wahoo
def vm(name, sock1, sock2, name1, name2):
    conn = []

    # file
    f = open(name + '.out', 'w')

    # rand clock rate
    num_actions = math.floor(random.random() * 6) + 1
    sleep_time = 1./num_actions

    # initialize LC
    lc = 0

    # initialize message queue
    queue = Queue.Queue()

    # do the bullet points
    end_time = num_actions * TIME_TO_RUN_SECONDS
    i = 0
    while i < end_time: # math
        queue_len = len(queue)
        (rec_message, proc_name) = receive_message(conn[0], queue)
        i = inc_time(i, f, sleep_time, end_time)
        if (rec_message != -1):
            (i, lc) = process_rec(rec_message, proc_name, i, lc, \
                             sleep_time, end_time, queue_len, f)
            continue
        (rec_message, proc_name) = receive_message(conn[1], queue)
        i = inc_time(i, f, sleep_time, end_time)
        if (rec_message != -1):
            (i, lc) = process_rec(rec_message, proc_name, i, lc, \
                            sleep_time, end_time, queue_len, f)
            continue
        action_type = math.floor(random.random() * 10)
        if (action_type == 0):
            (i, lc) = process_send_one(conn[0], name1, i, lc, \
                            sleep_time, end_time, f)
        elif (action_type == 1):
            (i, lc) = process_send_one(conn[1], name2, i, lc, \
                            sleep_time, end_time, f)
        elif (action_type == 2):
            (i, lc) = process_send_both(conn, [name1, name2], i, lc, \
                            sleep_time, end_time, f)
        else:
            lc += 1
            i = inc_time(i, f, sleep_time, end_time)
            f.write("Internal event. \nLogical clock: " + str(lc) + \
                    "\nSystem time: " + global_time + "\n\n")
            i = inc_time(i, f, sleep_time, end_time)

    f.close()
    client.close()

if __name__ == '__main__':
    sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock1.bind(('',9090))
    sock1.listen(backlog=0)

    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serversocket.bind(('',9090))
    serversocket.listen(0)

    workers = [mp.Process(target=worker, args=(socket1,socket2)) for i in
            range(num_workers)]

    for p in workers:
        p.daemon = True
        p.start()

    while True:
        try:
            time.sleep(10)
        except:
            break