
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define debug(fmt,args...)	printf(fmt, ##args)
#define info(fmt,args...)	printf(fmt, ##args)

#define GPIO_BASE	0x01c20800
#define GPIO_SIZE	0x0100

void *reg_base;

#define CFG(A, b)	(((A)-'A')*0x24 + ((b)/8)*4)
#define DAT(A)		(*(volatile unsigned int*)(reg_base + ((A)-'A')*0x24 + 0x10))

#define fatal(fmt,args...)      _fatal(__func__, __LINE__, fmt, ##args)

void _fatal (const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	fprintf (stderr, "fatal @ %s(), line %d. %s(%d)\n", func, line, strerror (errno), errno);
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);

	exit (1);
}

void *map (unsigned long addr, int size)
{
	int fd;
	void *ret;
	long pa_addr;
	long page_size;

	page_size = sysconf(_SC_PAGE_SIZE);
	pa_addr = addr & (page_size - 1);

	debug ("page_size %08lx, addr %08lx, pa_addr %08lx\n", page_size, addr, pa_addr);

	fd = open ("/dev/mem", O_RDWR);
	if (fd < 0)
		fatal ("/dev/mem open failed.\n");

	ret = mmap (NULL, size + pa_addr, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr - pa_addr);
	if (ret == MAP_FAILED)
		fatal ("mmap() failed\n");

	close (fd);

	return ret + pa_addr;
}

unsigned int RD_CFG (unsigned int A, unsigned int b)
{
	unsigned int val;
	
	val = *(volatile unsigned int *)(reg_base + CFG(A, b));

	return (val >> (b & 0x7) * 4) & 0xf;
}

unsigned int WR_CFG (unsigned int A, unsigned int b, unsigned int val)
{
	unsigned int tmp;
	
	tmp = *(volatile unsigned int *)(reg_base + CFG(A, b));

	tmp &= ~(0x7U << (b&0x7)*4);
	tmp |=  (val  << (b&0x7)*4);

	*(volatile unsigned int *)(reg_base + CFG(A, b)) = tmp;

	return 0;
}

int dump (void)
{
	int A;
	int i;
	int b;

	A = 'A';
	for (b=0; b<=1; b++)
		printf ("%c%02d : CFG %d\n", A, b, RD_CFG (A, b));
	for (b=11; b<=17; b++)
		printf ("%c%02d : CFG %d\n", A, b, RD_CFG (A, b));

	A = 'G';
	for (b=6; b<=9; b++)
		printf ("%c%02d : CFG %d\n", A, b, RD_CFG (A, b));

	printf ("DAT %c : 0x%08x\n", 'A', DAT('A'));
	printf ("DAT %c : 0x%08x\n", 'G', DAT('G'));
}

int got_sig;

void sighandle (int num)
{
	printf ("got signal %d\n", num);
	got_sig ++;
}

int main (int argc, char **argv)
{
	unsigned int tmpA, tmpG;
	int i;

	signal (SIGINT, sighandle);

	reg_base = map (GPIO_BASE, GPIO_SIZE);

	info ("gpio base %p\n", reg_base);

	dump ();

	WR_CFG ('A', 0, 1);
	WR_CFG ('A', 1, 1);

	WR_CFG ('A', 11, 1);
	WR_CFG ('A', 12, 1);
	WR_CFG ('A', 13, 1);
	WR_CFG ('A', 14, 1);
	WR_CFG ('A', 15, 1);
	WR_CFG ('A', 16, 1);
	WR_CFG ('A', 17, 1);

	WR_CFG ('G', 6, 1);
	WR_CFG ('G', 7, 1);
	WR_CFG ('G', 8, 1);
	WR_CFG ('G', 9, 1);

	dump ();

	tmpA = DAT ('A');
	tmpA &= 0xfffc07ffU;
	printf ("tmpA %08x\n", tmpA);

	tmpG = DAT ('G');
	tmpG &= 0xfffffc3fU;
	printf ("tmpG %08x\n", tmpG);

	while(!got_sig)
	{
		unsigned int d;

		d = tmpA | (i << 11);
		DAT ('A') = d;

		d = tmpG | (i << 6);
		DAT ('G') = d;

		usleep (10000);

		i++;
	}

	DAT ('A') = tmpA;
	DAT ('G') = tmpG;
	printf ("done.\n");

	return 0;
}
