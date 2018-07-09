
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
#define PUL(A,b)	(((A)-'A')*0x24 + 0x1c + (((b)/16)*4))

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

unsigned int RD_PUL (unsigned int A, unsigned int b)
{
	unsigned int val;
	
	val = *(volatile unsigned int *)(reg_base + PUL(A, b));

	return (val >> (b & 0xf) * 2) & 0x3;
}

unsigned int WR_PUL (unsigned int A, unsigned int b, unsigned int val)
{
	unsigned int tmp;
	
	tmp = *(volatile unsigned int *)(reg_base + PUL(A, b));

	tmp &= ~(0x3U << (b&0xf)*2);
	tmp |=  (val  << (b&0xf)*2);

	*(volatile unsigned int *)(reg_base + PUL(A, b)) = tmp;

	return 0;
}

unsigned int orgA, orgG;

// portA
#define BIT_STO		0
#define BIT_OE		1
#define BIT_RGB		11
#define BIT_CLK		17

// portG
#define BIT_ABCD	6

int dump (void)
{
	int A;
	int i;
	int b;

#define print_conf(A,b)		printf("%c%02d : CFG %d, PUL %d\n", A, b, RD_CFG(A, b), RD_PUL(A, b))
	A = 'A';
	print_conf (A, BIT_STO);
	print_conf (A, BIT_OE);
	for (b=BIT_RGB; b<BIT_RGB+6; b++)
		print_conf (A, b);
	print_conf (A, BIT_CLK);

	A = 'G';
	for (b=BIT_ABCD; b<BIT_ABCD+4; b++)
		print_conf (A, b);

	printf ("DAT %c : 0x%08x\n", 'A', DAT('A'));
	printf ("DAT %c : 0x%08x\n", 'G', DAT('G'));
}

int scan_row (void)
{
	static int row;

	int i;
	unsigned int out;

	DAT ('A') = orgA | (1 << BIT_OE);

	DAT ('G') = orgG | (row << BIT_ABCD);
	//debug ("row %2d, G:0x%08x\n", row, orgG | (row << BIT_ABCD));

	for (i=0; i<32; i++)
	{
		unsigned int rgb;

		rgb = 0;

		//rgb |= (0x01 | 0x08);
		if (((i + row + 0) % 8) == 0) rgb |= (0x01 | 0x08);
		if (((i + row + 1) % 8) == 0) rgb |= (0x02 | 0x10);
		if (((i + row + 2) % 8) == 0) rgb |= (0x04 | 0x20);

		out = orgA | (rgb << BIT_RGB) | (1 << BIT_OE);
		DAT ('A') = out;
		DAT ('A') = out | (1U << BIT_CLK);
	}

	DAT ('A') = out  | (1U < BIT_CLK);
	DAT ('A') = out  | (1U < BIT_CLK) | (1 << BIT_STO);
	DAT ('A') = orgA | (1U < BIT_CLK) | (1 << BIT_STO);

	row ++;
	row &= 0xf;

	return 0;
}

int got_sig;
void sighandle (int num)
{
	printf ("got signal %d\n", num);
	got_sig ++;
}

int main (int argc, char **argv)
{
	int i;

	signal (SIGINT, sighandle);

	reg_base = map (GPIO_BASE, GPIO_SIZE);

	info ("gpio base %p\n", reg_base);

	dump ();

	WR_CFG ('A', BIT_STO, 1);
	WR_CFG ('A', BIT_OE, 1);

	for (i=BIT_RGB; i<BIT_RGB+6; i++)
		WR_CFG ('A', i, 1);
	WR_CFG ('A', BIT_CLK, 1);

	for (i=BIT_ABCD; i<BIT_ABCD+4; i++)
		WR_CFG ('G', i, 1);

	dump ();

	orgA = DAT ('A');
	orgA &= 0xfffc07fcU;
	//orgA |= 0x9U<<BIT_RGB;
	printf ("orgA %08x\n", orgA);

	orgG = DAT ('G');
	orgG &= 0xfffffc3fU;
	printf ("orgG %08x\n", orgG);

	DAT ('A') = orgA;
	DAT ('G') = orgG;

	while(!got_sig)
	{
#if 1
		scan_row ();
		usleep (1*1000);
#else
		DAT ('A') = orgA;
		printf ("clk off\n");
		sleep (2);

		DAT ('A') = orgA | (1U << BIT_CLK);
		printf ("clk on\n");
		sleep (2);
#endif
	}

	DAT ('A') = orgA | (1U << BIT_OE);
	DAT ('G') = orgG;
	printf ("done.\n");

	return 0;
}
