
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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

int read_all (int fd)
{
	int ret;
	struct pollfd fds = { };
	unsigned char buf[16];

	fds.fd = fd;
	fds.events = POLLIN;

	while (1)
	{
		fds.revents = 0;

		ret = poll (&fds, 1, 0);
		if (ret < 0)
		{
			printf ("poll failed. %d\n", ret);
			exit (1);
		}

		if (ret == 0)
		{
			return 0;
		}

		if (!(fds.revents & POLLIN))
		{
			printf ("no POLLIN. 0x%x\n", fds.revents);
			exit (1);
		}

		ret = read (fd, buf, sizeof (buf));
		if (ret <= 0)
		{
			printf ("read failed. %d. %s\n", ret, strerror (errno));
			exit (1);
		}
		printf ("got %d bytes\n", ret);
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
		const char *led_vacant;
		const char *led_occupied;

		switch (buf[2]&0x7)
		{
			default:
			case 0: led_vacant = "red:occupied, green:vacant";      break;
			case 1: led_vacant = "red:off,      green:off";         break;
			case 2: led_vacant = "red:off,      green:on";          break;
			case 3: led_vacant = "red:on,       green:off";         break;
			case 4: led_vacant = "red:on,       green:on";          break;
			case 5: led_vacant = "red:off,      green:blinking";    break;
			case 6: led_vacant = "red:blinking, green:off";         break;
			case 7: led_vacant = "red:blinking, green:blinking";    break;
		}

		switch (buf[3]&0x7)
		{
			default:
			case 0: led_occupied = "red:occupied, green:vacant";      break;
			case 1: led_occupied = "red:off,      green:off";         break;
			case 2: led_occupied = "red:off,      green:on";          break;
			case 3: led_occupied = "red:on,       green:off";         break;
			case 4: led_occupied = "red:on,       green:on";          break;
			case 5: led_occupied = "red:off,      green:blinking";    break;
			case 6: led_occupied = "red:blinking, green:off";         break;
			case 7: led_occupied = "red:blinking, green:blinking";    break;
		}

		printf ("address                    0x%02x\n", buf[0]);
		printf ("detecting interval time    0x%02x\n", buf[1]);
		printf ("control lamp when vacant   0x%02x(%s)\n", buf[2], led_vacant);
		printf ("control lamp when occupied 0x%02x(%s)\n", buf[3], led_occupied);
		printf ("DIP switch                 0x%02x\n", buf[4]);
#define two(a,b)	(((a)<<8)|(b))
		printf ("distance 0                 0x%02x%02x(%3dcm)\n", buf[ 5], buf[ 6], two (buf[ 5], buf[ 6]));
		printf ("distance 1                 0x%02x%02x(%3dcm)\n", buf[ 7], buf[ 8], two (buf[ 7], buf[ 8]));
		printf ("distance 2                 0x%02x%02x(%3dcm)\n", buf[ 9], buf[10], two (buf[ 9], buf[10]));
		printf ("distance 3                 0x%02x%02x(%3dcm)\n", buf[11], buf[12], two (buf[11], buf[12]));
		printf ("installation height        0x%02x%02x(%3dcm)\n", buf[13], buf[14], two (buf[13], buf[14]));
		printf ("parking status             0x%02x(%s)\n", buf[15], (buf[15]&0x01)?"occupied":"vacant");
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

	read_all (fd);
	write_data (fd, sync, addr, order);
	read_data (fd, order);

	return 0;
}
