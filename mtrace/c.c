
#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

int main (int argc, char **argv)
{
	void *p, *t;

	mtrace();

	t = malloc(0x10);
	p = malloc(0x20);
	t = malloc(0x30);

	free(p);

	return 0;
}

