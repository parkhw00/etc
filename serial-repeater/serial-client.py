#!/usr/bin/env python3

import socket
import fcntl
import termios
import tty
import sys
import os
import time
import select
import logging

logger = logging.getLogger("serial-client")


class NonBlockingInput(object):

    def __enter__(self):
        # canonical mode, no echo
        self.old = termios.tcgetattr(sys.stdin)
        # self.old = tty.setraw(sys.stdin.fileno(), termios.TCSADRAIN)
        # new = termios.tcgetattr(sys.stdin)
        # new[3] = new[3] & ~(termios.ICANON | termios.ECHO)
        # termios.tcsetattr(sys.stdin, termios.TCSADRAIN, new)

        # set for non-blocking io
        self.orig_fl = fcntl.fcntl(sys.stdin, fcntl.F_GETFL)
        fcntl.fcntl(sys.stdin, fcntl.F_SETFL, self.orig_fl | os.O_NONBLOCK)

    def __exit__(self, *args):
        # restore terminal to previous state
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old)

        # restore original
        fcntl.fcntl(sys.stdin, fcntl.F_SETFL, self.orig_fl)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--socket",         default="./socket",
                                            help="server socket name")
    parser.add_argument("--loglevel",       default="info",
                                            choices=["info", "debug"],
                                            help="log level")
    opts = parser.parse_args()

    # print(f"opts : {opts}")
    logger.setLevel(getattr(logging, opts.loglevel.upper()))
    ch = logging.StreamHandler()

    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(opts.socket)
    client.sendall(b"Hi...")

    with NonBlockingInput():
        ps = {}

        def f(data, event):
            if data:
                client.sendall(data)
        ps[sys.stdin.fileno()] = f

        def f(data, event):
            if data:
                sys.stdout.buffer.write(data)
            if event & select.POLLHUP:
                raise Exception("Connection lost.")
        ps[client.fileno()]    = f

        po = select.poll()
        for fd in ps:
            po.register(fd, select.POLLIN)

        while True:
            es = po.poll()
            logger.debug(f"es: {es}")
            for fd, event in es:
                if event & select.POLLIN:
                    data = os.read(fd, 1024)
                    logger.debug(f"fd {fd}, event {event}, data {data}")
                    ps[fd](data, event)

