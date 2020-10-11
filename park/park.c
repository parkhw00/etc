
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int write_data (int fd, unsigned int sync, unsigned int addr, unsigned int order)
{
	unsigned char buf[4];
	int ret;

	buf[0] = sync;
	buf[1] = addr;
	buf[2] = order;
	buf[3] = buf[0] ^ buf[1] ^ buf[2];

	ret = write (fd, buf, sizeof (buf));
	if (ret != sizeof (buf))
	{
		printf ("write failed. %d\n", ret);
		exit (1);
	}

	return 0;
}

unsigned int get_read_size (unsigned int order)
{
	switch (order)
	{
		default: return 2;
		case 0xa6: return 16;
	}
}

int read_data (int fd, unsigned int order)
{
	int ret;
	unsigned int read_size = get_read_size (order);
	unsigned char buf[read_size];
	int filled;
	int i;

	filled = 0;
	while (filled < read_size)
	{
		struct pollfd fds = { };

		fds.fd = fd;
		fds.events = POLLIN;

		ret = poll (&fds, 1, 1000);
		if (ret < 0)
		{
			printf ("poll failed. %d\n", ret);
			exit (1);
		}

		if (ret == 0)
		{
			printf ("poll timeout.\n");
			exit (1);
		}

		if (!(fds.revents & POLLIN))
		{
			printf ("no POLLIN. 0x%x\n", fds.revents);
			exit (1);
		}

		ret = read (fd, buf + filled, read_size - filled);
		if (ret <= 0)
		{
			printf ("read failed. %d\n", ret);
			exit (1);
		}

		filled += ret;
	}

	printf ("%02x:", order);
	for (i=0; i<filled; i++)
		printf (" %02x", buf[i]);
	printf ("\n");
	if (order == 0xa6)
	{
		printf ("address                    0x%02x\n", buf[0]);
		printf ("detecting interval time    0x%02x\n", buf[1]);
		printf ("control lamp when vacant   0x%02x\n", buf[2]);
		printf ("control lamp when occupied 0x%02x\n", buf[3]);
		printf ("DIP switch                 0x%02x\n", buf[4]);
#define two(a,b)	(((a)<<8)|(b))
		printf ("distance 0                 0x%02x%02x(%3dcm)\n", buf[ 5], buf[ 6], two (buf[ 5], buf[ 6]));
		printf ("distance 1                 0x%02x%02x(%3dcm)\n", buf[ 7], buf[ 8], two (buf[ 7], buf[ 8]));
		printf ("distance 2                 0x%02x%02x(%3dcm)\n", buf[ 9], buf[10], two (buf[ 9], buf[10]));
		printf ("distance 3                 0x%02x%02x(%3dcm)\n", buf[11], buf[12], two (buf[11], buf[12]));
		printf ("installation height        0x%02x%02x(%3dcm)\n", buf[13], buf[14], two (buf[13], buf[14]));
		printf ("parking status             0x%02x\n", buf[15]);
	}
	if (order == 0xa5)
	{
		switch (buf[1])
		{
			case 0xe2:
				printf ("occupied\n");
				break;

			case 0xe1:
				printf ("vacant\n");
				break;

			default:
				printf ("unknown\n");
				break;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct termios termios = { };
	int fd;
	char *name = "/dev/ttyUSB0";
	unsigned int sync = 0xa3;
	unsigned int addr = 0x01;
	unsigned int order = 0xa2;

	while (1)
	{
		int opt;

		opt = getopt (argc, argv, "s:d:a:o:");
		if (opt < 0)
			break;
		switch (opt)
		{
			default:
				printf ("unknown option.\n");
				printf (
" park [<option> ...]\n"
"\n"
" options:\n"
" -d <tty name>\n"
" -s <sync byte>    0xa3 for default commands,\n"
"                   0xb1 to set installation height\n"
" -a <device address>\n"
" -o <order>\n"
					);
				exit (1);

			case 's':
				sync = strtoul (optarg, NULL, 0);
				break;

			case 'd':
				name = optarg;
				break;

			case 'a':
				addr = strtoul (optarg, NULL, 0);
				break;

			case 'o':
				order = strtoul (optarg, NULL, 0);
				break;
		}
	}

	fd = open (name, O_RDWR);
	if (fd < 0)
	{
		printf ("open failed. %s\n", name);
		return 1;
	}

	tcgetattr(fd, &termios);
	termios.c_lflag &= ~(ECHO | ICANON);
	cfsetspeed (&termios, B4800);
	tcsetattr(fd, TCSAFLUSH, &termios);

	write_data (fd, sync, addr, order);
	read_data (fd, order);

	return 0;
}
