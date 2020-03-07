
#include "lxlog.h"

lxlog_define (ttt);

int main (int argc, char **argv)
{
    printf ("first..\n");

    lx_debug (ttt, "hi.. %d\n", 5);
    return 0;
}
