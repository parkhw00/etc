
#include <poll.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int debuglevel;

#define debug(...)  do{if(debuglevel > 0) fprintf(stderr, __VA_ARGS__);}while(0)
#define fatal(...)  _fatal(__func__, __LINE__, __VA_ARGS__)
void _fatal(const char *func, int line, const char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%s.%d: fatal erron.\n", func, line);

    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    exit(1);
}

int npfds;
struct pollfd *pfds;

typedef int (*eventcb_t)(void *arg, int fd, short revents);
struct cbs
{
    eventcb_t cb;
    void *cb_arg;
} *cbs;

int add_pollfd(int fd, short events, eventcb_t cb, void *cb_arg)
{
    int i;
    for (i=0; i<npfds; i++)
        if (pfds[i].fd < 0)
            break;
    if (i == npfds)
    {
        npfds ++;
        debug("realloc memory.. %d\n", npfds);
        pfds = realloc(pfds, npfds * sizeof(pfds[0]));
        cbs = realloc(cbs, npfds * sizeof(pfds[0]));
    }

    debug("add index %d, fd %d\n", i, fd);
    pfds[i].fd = fd;
    pfds[i].events = events;
    pfds[i].revents = 0;
    cbs[i].cb = cb;
    cbs[i].cb_arg = cb_arg;
    return 0;
}

int rm_pollfd(int fd)
{
    int i;
    for (i=0; i<npfds; i++)
    {
        if (pfds[i].fd == fd)
        {
            debug("rm  index %d, fd %d\n", i, fd);
            pfds[i].fd = -1;
            pfds[i].events = 0;
            return 0;
        }
    }

    fatal("unknown fd. %d\n", fd);
    return 0;
}

int server_socket(char *path)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        fatal("\n");

    unlink(path);
    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        fatal("bind() failed\n");

    if (listen(sock, 32) < 0)
        fatal("listen() failed\n");

    return sock;
}

int client_event(void *arg, int fd, short revents)
{
    debug("got client event. fd %d, revents %d\n", fd, revents);
    if (revents & POLLIN)
    {
        unsigned char buf[1024];
        ssize_t got;

        got = read(fd, buf, sizeof(buf));
        debug("read() %zd bytes\n", got);
    }

    if (revents & POLLHUP)
    {
        debug("hup...\n");
        close(fd);
        rm_pollfd(fd);
    }

    return 0;
}

int server_event(void *arg, int fd, short revents)
{
    debug("got server event. fd %d, revents %d\n", fd, revents);
    if (revents & POLLIN)
    {
        struct sockaddr_un addr = { };
        socklen_t len = sizeof(addr);
        int client = accept(fd, (struct sockaddr*)&addr, &len);
        if (client < 0)
            fatal("accept() failed\n");

        add_pollfd(client, POLLIN, client_event, NULL);
    }

    return 0;
}

int main(int argc, char **argv)
{
    while (1)
    {
        int opt;
        opt = getopt(argc, argv, "D");
        if (opt == -1)
            break;
        switch (opt)
        {
            default:
                exit(1);

            case 'D':
                debuglevel ++;
                break;
        }
    }

    add_pollfd(server_socket("./socket"), POLLIN, server_event, NULL);

    while(1)
    {
        int ret;
        ret = poll(pfds, npfds, -1);
        debug("poll return %d\n", ret);

        int i;
        for (i=0; i<npfds; i++)
        {
            if (pfds[i].fd >= 0 && pfds[i].revents != 0)
                cbs[i].cb(cbs[i].cb_arg, pfds[i].fd, pfds[i].revents);
        }
    }

    return 0;
}
