#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <malloc.h>
#include <execinfo.h>

extern void *__libc_malloc(size_t size);
extern void __libc_free(void *ptr);

static int num_frame = 8;
static bool internal;
static bool term;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

extern void mdb_add (unsigned long ptr, size_t size, void **bt, int num);
extern void mdb_del (unsigned long ptr);

static void dump_remains (void) __attribute__((destructor));
static void dump_remains (void)
{
  extern void mdb_dump_remains (void);

  term = true;

  printf ("exit...\n");
  mdb_dump_remains ();
}

void *malloc (size_t size)
{
  void *result;
  int locked;

  if (term)
    return __libc_malloc (size);

  locked = pthread_mutex_trylock (&lock);
  if (locked == 0)
  {
    static bool initialized;

    if (!initialized)
    {
      initialized = true;
      //atexit (dump_remains);
    }

    if (!internal)
    {
      void *bt[num_frame];
      int num;

      internal = true;
      memset (bt, 0, sizeof (void*) * num_frame);
      num = backtrace (bt, num_frame);

      result = __libc_malloc (size);

      mdb_add ((unsigned long)result, size, bt, num);
      if (0)
      {
        int i;
        char **bts;

        printf ("%p, malloc(%zd)\n", result, size);
        bts = backtrace_symbols (bt, num);
        for (i=0; i<num; i++)
          printf ("  %d. %p, %s\n", i, bt[i], bts[i]);

        free (bts);
      }
      internal = false;
      pthread_mutex_unlock (&lock);

      return result;
    }
    abort ();
  }
  else if (locked != EBUSY)
    abort ();

  return __libc_malloc (size);
}

void free (void *ptr)
{
  if (term)
    return __libc_free (ptr);

  if (pthread_mutex_trylock (&lock) == 0)
  {
    if (!internal)
    {
      if (0)
        printf ("%p, free()\n", ptr);
      mdb_del ((unsigned long)ptr);
      pthread_mutex_unlock (&lock);
    }
    else
      abort ();
  }
  __libc_free (ptr);
}

#if 0
static void init (void) __attribute__((constructor));
static void init (void)
{
  atexit (dump_remains);
}
#endif

// vim:set sw=2 et:
