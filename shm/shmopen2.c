
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main (int argc, char **argv)
{
	int fd;
	char *name = "shmname";
	void *mem;
	int ret;
	int size = 0x1000;

	if (argc >= 2)
		name = argv[1];

	printf ("name %s\n", name);
	fd = shm_open (name, O_RDWR, 0660);
	if (fd < 0)
	{
		printf ("shm_open() failed. %d(%s)\n", errno, strerror (errno));
		exit (1);
	}

	mem = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED)
	{
		printf ("mmap() failed. %d(%s)\n", errno, strerror (errno));
		exit (1);
	}

	close (fd);

	printf ("Message from Shared Memory. \"\"%s\"\"\n", (char*)mem);

	munmap (mem, size);

	return 0;
}
