#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include "link.h"

#define debug(fmt,args...)	printf(fmt, ##args)
#define info(fmt,args...)	printf(fmt, ##args)
#define error(fmt,args...)	fprintf(stderr, fmt, ##args)

#define fatal(fmt,args...)	_fatal(__func__, __LINE__, fmt, ##args)

void _fatal (const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	fprintf (stderr, "fatal @ %s(), line %d. %s(%d)\n", func, line, strerror (errno), errno);

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);

	exit (1);
}

int poll_nfds;
DEFINE_LINK (poll_root_fds);
struct poll_fdinfo
{
	struct link link;

	int fd;
	int events;
	int (*handle) (struct poll_fdinfo *info, int revents);
	void *priv;
};

int poll_new (int fd, int events, int (*handle) (struct poll_fdinfo *info, int revents), void *priv)
{
	struct poll_fdinfo *info;

	info = calloc (1, sizeof (*info));
	if (!info)
		fatal ("no mem for pollfd\n");

	info->fd = fd;
	info->events = events;
	info->handle = handle;
	info->priv = priv;
	link_add_tail (&poll_root_fds, &info->link);

	poll_nfds ++;

	return 0;
}

int poll_del (struct poll_fdinfo *info)
{
	link_del (&info->link);
	free (info);
	poll_nfds --;

	return 0;
}

struct pollfd *poll_fds (int *pnfds)
{
	static struct pollfd *fds;
	static int nfds;

	struct poll_fdinfo *now;
	int i;

	if (nfds < poll_nfds)
	{
		fds = realloc (fds, sizeof (fds[0]) * poll_nfds);
		if (!fds)
			fatal ("no mem for fds. %d\n", poll_nfds);
		nfds = poll_nfds;
	}

	i = 0;
	link_for_each (&poll_root_fds, now, link)
	{
		fds[i].fd = now->fd;
		fds[i].events = now->events;
		fds[i].revents = 0;

		debug ("fd %d, events %d\n", now->fd, now->events);

		i ++;
	}

	debug ("nfds %d\n", i);
	*pnfds = i;
	return fds;
}

int poll_handle (struct pollfd *fds, int nfds)
{
	int i;

	for (i=0; i<nfds; i++)
	{
		struct poll_fdinfo *now, *sel;

		if (!fds[i].revents)
			continue;

		sel = NULL;
		link_for_each (&poll_root_fds, now, link)
		{
			if (now->fd == fds[i].fd)
			{
				sel = now;
				break;
			}
		}

		if (!sel)
			fatal ("Oops??\n");

		debug ("handle fd %d, revents %d\n", sel->fd, fds[i].revents);
		sel->handle (sel, fds[i].revents);
	}

	return 0;
}

int handle_client (struct poll_fdinfo *info, int revents)
{
	if (revents & POLLHUP)
	{
		poll_del (info);
		return 0;
	}

	if (revents & POLLIN)
	{
		char buf[256];
		int ret;

		ret = read (info->fd, buf, sizeof (buf) - 1);
		if (ret < 0)
		{
			error ("read() failed. %s(%d)\n", strerror (errno), errno);
			return 0;
		}

		buf[ret] = 0;
		info ("got %s\n", buf);

		ret = write (info->fd, "tttest", 6);
	}

	return 0;
}

int handle_socket (struct poll_fdinfo *info, int revents)
{
	if (revents & POLLIN)
	{
		int infd;

		infd = accept (info->fd, NULL, NULL);
		if (infd < 0)
		{
			error ("accept() failed. %s(%d)\n", strerror (errno), errno);
			return 0;
		}

		poll_new (infd, POLLIN, handle_client, NULL);
	}

	return 0;
}

int main (int argc, char **argv)
{
	int fd;
	struct sockaddr_un name;
	char *socket_name = "sock";
	int ret;

	unlink (socket_name);

	fd = socket (AF_UNIX, SOCK_SEQPACKET, 0);
	if (fd < 0)
		fatal ("socket() failed\n");

	memset (&name, 0, sizeof (name));
	name.sun_family = AF_UNIX;
	strncpy (name.sun_path, socket_name, sizeof (name.sun_path) - 1);

	if (bind (fd, (const struct sockaddr*)&name, sizeof (struct sockaddr_un)) < 0)
		fatal ("bind() failed.\n");

	if (listen (fd, 20) < 0)
		fatal ("listen() failed.\n");

	poll_new (fd, POLLIN, handle_socket, NULL);

	while (1)
	{
		struct pollfd *fds;
		int nfds;

		fds = poll_fds (&nfds);
		ret = poll (fds, nfds, -1);
		if (ret < 0)
			fatal ("poll() failed.\n");
		debug ("poll ret %d\n", ret);

		poll_handle (fds, nfds);
	}

	return 0;
}

