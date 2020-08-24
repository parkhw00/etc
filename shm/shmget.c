
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int print_shm (int id)
{
	int ret;
	struct shmid_ds buf = { };

	printf ("id %d(0x%x)\n", id, id);

	ret = shmctl (id, IPC_STAT, &buf);
	if (ret < 0)
	{
		printf ("shmctl(IPC_STAT) error %d. %s\n", errno, strerror (errno));
		exit (1);
	}

#define pr(n)	printf (#n ": %d\n", buf.n);
	pr (shm_perm.__key);
	pr (shm_perm.uid);
	pr (shm_perm.gid);
	pr (shm_perm.cuid);
	pr (shm_perm.cgid);
	pr (shm_perm.mode);
	pr (shm_perm.__seq);
	pr (shm_segsz);
	pr (shm_atime);
	pr (shm_dtime);
	pr (shm_ctime);
	pr (shm_cpid);
	pr (shm_lpid);
	pr (shm_nattch);

	return 0;
}

int main (int argc, char **argv)
{
	int ret;
	int id;
	void *mem;

	id = shmget (IPC_PRIVATE, 0x1000, IPC_CREAT);
	printf ("id %d(0x%x)\n", id, id);
	print_shm (id);

	mem = shmat (id, NULL, 0);
	printf ("mem %p\n", mem);
	if (mem == (void*)-1)
	{
		printf ("error %d. %s\n", errno, strerror (errno));
		exit (1);
	}
	sprintf (mem, "id %d(0x%x), mem %p", id, id, mem);

	ret = shmdt (mem);
	printf ("dt ret %d\n", ret);

	ret = shmctl (id, IPC_RMID, NULL);
	printf ("rmid ret %d\n", ret);

	return 0;
}
