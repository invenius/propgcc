#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sys/syscall.h"

int errno;

int __trap0 (int function, int p1, int p2, int p3);

#define TRAP0(f, p1, p2, p3) __trap0(f, (int)(p1), (int)(p2), (int)(p3))

int
_fstat (int file,
	struct stat *st)
{
  return TRAP0 (SYS_fstat, file, st, 0);
}
