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

logger = logging.getLogger("serial-sock")


class NonBlockingInput(object):

    def __enter__(self):
        self.old = termios.tcgetattr(sys.stdin)
        self.old = tty.setraw(sys.stdin.fileno(), termios.TCSADRAIN)

        self.orig_fl = fcntl.fcntl(sys.stdin, fcntl.F_GETFL)
        fcntl.fcntl(sys.stdin, fcntl.F_SETFL, self.orig_fl | os.O_NONBLOCK)

    def __exit__(self, *args):
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old)
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

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(opts.socket)
    sock.sendall(b"Hi...")

    with NonBlockingInput():
        ps = {}

        got_special = False
        running = True
        def f(data, event):
            if data:
                global got_special, running
                if not got_special and data == b'\x1b':
                    got_special = True
                    logger.debug("got special key. ^[")
                elif got_special and data == b'q':
                    logger.debug("quit.")
                    running = False
                else:
                    got_special = False
                    sock.sendall(data)
        ps[sys.stdin.fileno()] = f

        def f(data, event):
            if data:
                sys.stdout.buffer.write(data)
            if event & select.POLLHUP:
                raise Exception("Connection lost.")
        ps[sock.fileno()] = f

        po = select.poll()
        for fd in ps:
            po.register(fd, select.POLLIN)

        while running:
            es = po.poll()
            logger.debug(f"es: {es}")
            for fd, event in es:
                data = None
                if event & select.POLLIN:
                    data = os.read(fd, 1024)
                logger.debug(f"fd {fd}, event {event}, data {data}")
                ps[fd](data, event)

