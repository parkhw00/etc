
#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

class aaa
{
	int member[0x10];
};

class bbb
{
	int member[0x20];
};

class ccc
{
	int member[0x30];
};

int main (int argc, char **argv)
{
	mtrace ();

	class aaa *a = new aaa();
	class bbb *b = new bbb();
	class ccc *c = new ccc();

	delete b;

	return 0;
}
