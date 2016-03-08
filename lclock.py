import multiprocessing as mp
import logging
import socket
import time


TIME_TO_RUN_SECONDS = 60

# wahoo
def vm(sock1, sock2):

    # file

    # rand clock rate

    # do the bullet points

    while True: # math
        client, address = socket.accept()
        logger.debug("{u} connected".format(u=address))
        client.send("OK")
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