
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

struct aframe
{
  unsigned int user;
  unsigned long faddr;
  unsigned int depth;
  unsigned long alloc_ptr;

  struct aframe *callee;

  unsigned int callers_allocated;
  unsigned int callers_filled;
  struct aframe *callers;
};

struct aframe_ptr
{
  unsigned long ptr;
  size_t size;
  struct aframe *frame;

  struct aframe_ptr *next;
};

static struct aframe_ptr *pairdb[256];
static void add_ptr (unsigned long ptr, size_t size, struct aframe *frame)
{
  unsigned int hindex;
  unsigned long tmp = ptr;
  struct aframe_ptr **pair;
  int i;

  hindex = 0;
  for (i=0; i < sizeof (tmp) * 8 / 8; i++)
    hindex ^= (tmp >> (i*8)) & 0xff;
  pair = &pairdb[hindex];

do_next:
  if (*pair == NULL)
  {
    *pair = calloc (1, sizeof (struct aframe_ptr));
    (*pair)->ptr = ptr;
    (*pair)->size = size;
    (*pair)->frame = frame;
    return;
  }
  pair = &((*pair)->next);
  goto do_next;
}

static struct aframe root;

static void insert (struct aframe *frame, unsigned int depth,
    unsigned long ptr, size_t size, void **bt, int num)
{
  struct aframe *cframe;
  int s, e;
  int cindex;

  s = 0;
  e = frame->callers_filled;
  while (s != e)
  {
    int m;

    m = s + (e - s) / 2;
    if (frame->callers[m].faddr == (unsigned long)bt[0])
    {
      cframe = frame->callers + m;
      goto got_match;
    }

    if (frame->callers[m].faddr < (unsigned long)bt[0])
      s = m + 1;
    else
      e = m;
  }

  if (frame->callers_filled + 1 > frame->callers_allocated)
  {
    frame->callers_allocated += 256;
    frame->callers = realloc (frame->callers,
        sizeof (struct aframe) * frame->callers_allocated);
  }
  if (s < frame->callers_filled)
    memmove (frame->callers + s + 1, frame->callers + s,
        sizeof (struct aframe) * (frame->callers_filled - s));

  frame->callers_filled ++;
  cframe = frame->callers+s;
  cframe->faddr = (unsigned long)bt[0];
  cframe->depth = depth + 1;
  cframe->callee = frame;

got_match:
  cframe->user ++;

  if (num == 1)
  {
    add_ptr (ptr, size, cframe);
    return;
  }

  insert (cframe, depth + 1, ptr, size, bt+1, num-1);
}

void mdb_add (unsigned long ptr, size_t size, void **bt, int num)
{
  if (!ptr)
    return;

  if (num > 256)
    num = 256;
  insert (&root, 0, ptr, size, bt, num);
}

void mdb_del (unsigned long ptr)
{
  unsigned int hindex;
  struct aframe_ptr **pair;
  int i;

  if (!ptr)
    return;

  hindex = 0;
  for (i=0; i < sizeof (ptr) * 8 / 8; i++)
    hindex ^= (ptr >> (i*8)) & 0xff;
  pair = &pairdb[hindex];

do_next:
  //assert (*pair);
  if (*pair == NULL)
  {
    fprintf (stderr, "%s.%d: unknown addr. 0x%x\n", __func__, __LINE__, ptr);
    return;
  }
  if ((*pair)->ptr == ptr)
  {
    struct aframe_ptr *del;
    struct aframe *frame;

    del = *pair;
    *pair = (*pair)->next;

    frame = del->frame;
    do
    {
      assert (frame->user > 0);
      frame->user --;
      frame = frame->callee;
    }
    while (frame->faddr);

    free (del);
    return;
  }

  pair = &((*pair)->next);
  goto do_next;
}

static void do_dump (void **bt, int num, struct aframe *frame)
{
  int i;

  for (i=0; i<frame->callers_filled; i++)
  {
    struct aframe *cframe = &frame->callers[i];

    //printf ("depth%d, faddr %x, user %d\n", num, cframe->faddr, cframe->user);
    if (cframe->user == 0)
      continue;

    bt[num] = (void*)cframe->faddr;
    if (cframe->callers_filled == 0)
    {
      int i;
      char **bts;

      fprintf (stderr, "not freed count %d\n", cframe->user);
      bts = backtrace_symbols (bt, num+1);
      if (bts)
      {
        for (i=0; i<num+1; i++)
          fprintf (stderr, "  %d. %p, %s\n", i, bt[i], bts[i]);

        free (bts);
      }
    }
    else
      do_dump (bt, num+1, cframe);
  }
}

void mdb_dump_remains (void)
{
  void *bt[256];
  unsigned int total_nonfreed;
  int i;

  do_dump (bt, 0, &root);

  for (i=0; i<(sizeof(pairdb)/sizeof(pairdb[0])); i++)
  {
    struct aframe_ptr **p;

    if (!pairdb[i])
      continue;

    p = &pairdb[i];
    while ((*p)->next)
    {
      p = &((*p)->next);
      total_nonfreed += (*p)->size;
    }
  }
  fprintf (stderr, "nonfree total %d\n", total_nonfreed);
}

// vim:set sw=2 et:
