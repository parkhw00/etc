
// https://www.gnu.org/software/tar/manual/html_node/Standard.html
//struct posix_header
struct tar_header
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};

#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  6
#define TVERSION "00"           /* 00 and no null */
#define TVERSLEN 2

/* Values used in typeflag field.  */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

#define XHDTYPE  'x'            /* Extended header referring to the
                                   next file in the archive */
#define XGLTYPE  'g'            /* Global extended header */

#define OLDGNU_MAGIC "ustar  "  /* 7 chars and a null */


/* This is a dir entry that contains the names of files that were in the
   dir at the time the dump was made.  */
#define GNUTYPE_DUMPDIR 'D'

/* Identifies the *next* file on the tape as having a long linkname.  */
#define GNUTYPE_LONGLINK 'K'

/* Identifies the *next* file on the tape as having a long name.  */
#define GNUTYPE_LONGNAME 'L'

/* This is the continuation of a file that began on another volume.  */
#define GNUTYPE_MULTIVOL 'M'

/* This is for sparse files.  */
#define GNUTYPE_SPARSE 'S'

/* This file is a tape/volume header.  Ignore it on extraction.  */
#define GNUTYPE_VOLHDR 'V'

/* Solaris extended header */
#define SOLARIS_XHDTYPE 'X'


#include <stdio.h>
#include <string.h>

#define error(...)	printf(__VA_ARGS__)
#define debug(...)	printf(__VA_ARGS__)

#define get_octal(header, member)	_get_octal((header)->member, sizeof((header)->member))
unsigned int _get_octal(char *str, unsigned int len)
{
	unsigned int ret;
	int i;

	ret = 0;
	for (i=0, ret=0; i<len; i++)
	{
		unsigned char t;

		if (str[i] == 0)
			break;
		t = str[i] - '0';
		if (t > 7)
		{
			error("wrong octal number. str %s\n", str);
			break;
		}

		ret <<= 3;
		ret += t;
	}

	return ret;
}

static int check_header(struct tar_header *header)
{
	struct tar_header tmp;
	unsigned int chksum, hchksum;
	unsigned char *btmp;
	int i;

	memcpy (&tmp, header, sizeof(tmp));
	memset (tmp.chksum, ' ', sizeof(tmp.chksum));

	chksum = 0;
	btmp = (unsigned char *)&tmp;
	for (i=0; i<sizeof(*header); i++)
		chksum += btmp[i];

	if (chksum == ' ' * sizeof(tmp.chksum))
		return -2;

	hchksum = get_octal(header, chksum);
	if (hchksum != chksum)
	{
		error ("wrong checksum. %d, %d\n", hchksum, chksum);
		return -1;
	}

	return 0;
}

static char type_to_ls(char type)
{
	switch (type)
	{
		case SYMTYPE:  return 'l';
		case DIRTYPE:  return 'd';
		case CHRTYPE:  return 'c';
		case BLKTYPE:  return 'b';
		case REGTYPE:
		case AREGTYPE: return '-';

		default: return type;
	}
}

unsigned int tar_parse(unsigned char *data, int size)
{
	int offs;
	char *filename;

	filename = NULL;
	offs = 0;
	while (size - offs >= 512)
	{
		struct tar_header *header;
		unsigned int member_size;
		unsigned int member_size_with_padding;
		unsigned int mode, mtime;

		header = (struct tar_header*)(data + offs);
		if (check_header(header) < 0)
			return -1;

		member_size = get_octal(header, size);

		if (header->typeflag == GNUTYPE_LONGNAME)
		{
			filename = (char *)data + offs + 512;
		}
		else
		{
			if (!filename)
				filename = header->name;

			mode = get_octal(header, mode);
			mtime = get_octal(header, mtime);
			printf("%c%c%c%c%c%c%c%c%c%c %s %s %6d %d %s",
					type_to_ls(header->typeflag),
					(mode&0400) ? 'r':'-',
					(mode&0200) ? 'w':'-',
					(mode&0100) ? 'x':'-',
					(mode&0040) ? 'r':'-',
					(mode&0020) ? 'w':'-',
					(mode&0010) ? 'x':'-',
					(mode&0004) ? 'r':'-',
					(mode&0002) ? 'w':'-',
					(mode&0001) ? 'x':'-',
					header->uname, header->gname,
					member_size, mtime, filename);

			if (header->typeflag == SYMTYPE)
				printf(" -> %s", header->linkname);
			printf("\n");

			filename = NULL;
		}

		member_size_with_padding = member_size + 512-1;
		member_size_with_padding &= ~(512-1);

		offs += 512 + member_size_with_padding;
	}

	error ("wrong size. %d\n", size);

	return -1;
}

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main (int argc, char **argv)
{
	int fd;
	void *data;
	unsigned int size;

	fd = open (argv[1], O_RDONLY);
	size = lseek (fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);
	data = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	close (fd);

	tar_parse (data, size);
	return 0;
}
