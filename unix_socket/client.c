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

int main (int argc, char **argv)
{
	int fd;
	struct sockaddr_un name;
	char *socket_name = "sock";
	int ret;
	char buf[32];

	fd = socket (AF_UNIX, SOCK_SEQPACKET, 0);
	if (fd < 0)
		fatal ("socket() failed\n");

	memset (&name, 0, sizeof (name));
	name.sun_family = AF_UNIX;
	strncpy (name.sun_path, socket_name, sizeof (name.sun_path) - 1);

	if (connect (fd, (const struct sockaddr*)&name, sizeof (struct sockaddr_un)) < 0)
		fatal ("connect() failed.\n");

	ret = write (fd, "test", 4);
	ret = read (fd, buf, sizeof (buf));
	if (ret < 0)
		fatal ("read() failed.\n");
	buf[ret] = 0;
	printf ("got %d, returns %s\n", ret, buf);

	return 0;
}

