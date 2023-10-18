#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int d = open("chardev", O_RDWR);
	char buf[4];
	read(d, buf, 4);
	ioctl(d, 0x1234, 0x5678);
	return 0;
}
