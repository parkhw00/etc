
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
	int id;
	void *mem;

	if (argc < 1)
		exit (1);

	id = atoi (argv[1]);
	printf ("id %d(0x%x)\n", id, id);

	mem = shmat (id, NULL, 0);
	printf ("mem %p: %s\n", mem, mem);

	return 0;
}
