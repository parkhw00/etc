
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>

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

/* return in microsecond */
static int get_time (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return tv.tv_sec * 1000000 + tv.tv_usec;
}

#define CBITS	4
unsigned char scanbuf[16][CBITS][64];

unsigned char *fb_data;

/* fb, ARGB 8888 format */
int convert_to_scanbuf (const unsigned char *fb)
{
	int W = 64;
	int H = 32;
	int stride = W * 4;

	int row, bit, col;

	for (row=0; row<16; row++)
	{
		for (col=0; col<64; col++)
		{
			unsigned char r0, g0, b0;
			unsigned char r1, g1, b1;

#if 1
			r0 = *(fb+(64*(row+ 0)+col)*4 + 2);
			g0 = *(fb+(64*(row+ 0)+col)*4 + 1);
			b0 = *(fb+(64*(row+ 0)+col)*4 + 0);

			r1 = *(fb+(64*(row+16)+col)*4 + 2);
			g1 = *(fb+(64*(row+16)+col)*4 + 1);
			b1 = *(fb+(64*(row+16)+col)*4 + 0);
#else
			r0 = g0 = b0 = 0;
			r1 = g1 = b1 = 0;
			if (((row - col) % 10) == 0)
				r0 = 0x80;
			if ((((row+16) - col) % 10) == 0)
				r1 = 0x10;

			if (((row - col) % 12) == 0)
				g0 = 0x40;
			if ((((row+16) - col) % 12) == 0)
				g1 = 0x10;

			if (((row - col) % 14) == 0)
				b0 = 0x20;
			if ((((row+16) - col) % 14) == 0)
				b1 = 0x10;
#endif

			for (bit=0; bit<4; bit++)
			{
				unsigned int out;

				out = 0;
				if (r0 & (0x80>>(CBITS-1-bit))) out |= 0x01;
				if (g0 & (0x80>>(CBITS-1-bit))) out |= 0x02;
				if (b0 & (0x80>>(CBITS-1-bit))) out |= 0x04;
				if (r1 & (0x80>>(CBITS-1-bit))) out |= 0x08;
				if (g1 & (0x80>>(CBITS-1-bit))) out |= 0x10;
				if (b1 & (0x80>>(CBITS-1-bit))) out |= 0x20;

				scanbuf[row][bit][col] = out;
			}
		}
	}
}

static int display_line (int row)
{
	DAT ('A') = orgA | (1U << BIT_OE);
	DAT ('G') = orgG | (row << BIT_ABCD);

	DAT ('A') = orgA | (1U << BIT_OE) | (1 << BIT_STO);
	DAT ('A') = orgA;

	return 0;
}

static int send_line (int row, int bit)
{
	int i;

	unsigned char *buf = scanbuf[row][bit];

	for (i=0; i<64; i++)
	{
		unsigned int out;

		out = orgA | (buf[i] << BIT_RGB);
		DAT ('A') = out;
		DAT ('A') = out | (1U << BIT_CLK);
	}

	return 0;
}

int got_sig;
#define HZ	60

#define ARRAY_SIZE(arr)		(sizeof(arr)/sizeof(arr[0]))
static int loop_scan (void)
{
	static const int on_tab[CBITS] =
	{
#define UNIT_DUR_HZ(n,HZ)	(n*1000000/HZ/16/((1<<CBITS)-1))
#define UNIT_DUR(n)		UNIT_DUR_HZ(n,HZ)
		UNIT_DUR(1),
		UNIT_DUR(2),
		UNIT_DUR(4),
		UNIT_DUR(8),
	};

	int frame_count;
	int next, prev;
	int row, bit;

	row = 0;
	bit = 0;
	frame_count = 0;
	next = get_time ();
	while(!got_sig)
	{
		int now;
		int sleep;
		int last_process;

		/* scan a row */
		prev = next;
		display_line (row);

		/* calculate next time, wait until next scan time */
		next += on_tab[bit];

		bit ++;
		if (bit >= CBITS)
		{
			bit = 0;
			row ++;
			if (row >= 16)
			{
				row = 0;
				frame_count ++;
				//printf ("%9dus, frame %d\n", now, frame_count);

				{
					int yoff;

					yoff = 1.*(128.-32.)*(1. - cos(2.*M_PI*0.25*frame_count/HZ))/2;

					if (yoff > 128-32)
						yoff = 128-32;
					if (yoff < 0)
						yoff = 0;

					//printf ("frame %d, yoff %d\n", frame_count, yoff);

					convert_to_scanbuf (fb_data + yoff*64*4);
				}
			}
		}

		send_line (row, bit);

		last_process = -1;
		while (1)
		{
			now = get_time ();
			if (last_process < 0)
				last_process = now - prev;

			sleep = next - now;
			if (sleep > 0)
			{
				//printf ("sleep %5dus\n", sleep);
				//usleep (sleep);
			}
			else
			{
				//printf ("sleep %3dus, last_process %3dus\n", sleep, last_process);
				break;
			}
		}
	}
}

int load_fb (const char *name)
{
#if 1
	int fd, size;

	fd = open (name, O_RDONLY);
	if (fd < 0)
		fatal ("cannot open %s\n", name);

	size = lseek (fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);

	fb_data = malloc (size);
	if (read (fd, fb_data, size) < 0)
		fatal ("read() failed.\n");

	convert_to_scanbuf (fb_data);
#else
	int i, j;

	fb_data = malloc (64*128*4);

	for (j=0; j<128; j++)
	{
		for (i=0; i<64; i++)
		{
			if (i < 8)
			{
				*(fb_data + (j*64 + i)*4 + 0) = j*2;
				*(fb_data + (j*64 + i)*4 + 1) = j*2;
				*(fb_data + (j*64 + i)*4 + 2) = j*2;
			}
			else if (i < 16)
			{
				*(fb_data + (j*64 + i)*4 + 0) = 0;
				*(fb_data + (j*64 + i)*4 + 1) = 0;
				*(fb_data + (j*64 + i)*4 + 2) = j*2;
			}
			else if (i < 24)
			{
				*(fb_data + (j*64 + i)*4 + 0) = 0;
				*(fb_data + (j*64 + i)*4 + 1) = j*2;
				*(fb_data + (j*64 + i)*4 + 2) = 0;
			}
			else if (i < 32)
			{
				*(fb_data + (j*64 + i)*4 + 0) = j*2;
				*(fb_data + (j*64 + i)*4 + 1) = 0;
				*(fb_data + (j*64 + i)*4 + 2) = 0;
			}
			else
			{
				*(fb_data + (j*64 + i)*4 + 0) = 0;
				*(fb_data + (j*64 + i)*4 + 1) = 0;
				*(fb_data + (j*64 + i)*4 + 2) = 0;
			}
		}
	}
#endif

	return 0;
}

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

	load_fb (argv[1]);

	//dump ();

	WR_CFG ('A', BIT_STO, 1);
	WR_CFG ('A', BIT_OE, 1);

	for (i=BIT_RGB; i<BIT_RGB+6; i++)
		WR_CFG ('A', i, 1);
	WR_CFG ('A', BIT_CLK, 1);

	for (i=BIT_ABCD; i<BIT_ABCD+4; i++)
		WR_CFG ('G', i, 1);

	//dump ();

	orgA = DAT ('A');
	orgA &= 0xfffc07fcU;
	//orgA |= 0x9U<<BIT_RGB;
	printf ("orgA %08x\n", orgA);

	orgG = DAT ('G');
	orgG &= 0xfffffc3fU;
	printf ("orgG %08x\n", orgG);

	DAT ('A') = orgA;
	DAT ('G') = orgG;

	loop_scan ();

	/* turn off all leds */
	DAT ('A') = orgA | (1U << BIT_OE);
	DAT ('G') = orgG;
	printf ("done.\n");

	return 0;
}
