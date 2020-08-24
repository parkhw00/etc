
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main (int argc, char **argv)
{
	int fd;
	char *name = "shmname";

	if (argc >= 2)
		name = argv[1];

	printf ("name %s\n", name);
	fd = shm_open (name, O_RDWR | O_CREAT | O_EXCL, 0660);
	if (fd < 0)
	{
		printf ("shm_open() failed. %d(%s)\n", errno, strerror (errno));
		exit (1);
	}

	//shm_unlink ("shmname");

	return 0;
}
