
https://man7.org/linux/man-pages/man3/mtrace.3.html
c 에서는 잘되는데, c++ 에서 new 연산자는 잘 안됨. backtrace 를 모두 저장하지 않음.

https://www.gnu.org/software/libc/manual/html_node/Hooks-for-Malloc.html
이거, deprecated 임.

https://stackoverflow.com/questions/17803456/an-alternative-for-the-deprecated-malloc-hook-functionality-of-glibc
이 방법이 좋을듯. 링크해도 되고, LD_PRELOAD 로도 사용할수 있을듯.
https://code.woboq.org/userspace/glibc/malloc/malloc.c.html
