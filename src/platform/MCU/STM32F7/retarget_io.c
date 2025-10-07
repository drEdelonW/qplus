#include <sys/errno.h>
#include <sys/stat.h>
// files
// int _close(int) { errno = ENOSYS; return -1; }
// int _fstat(int, struct stat* st) { if (st) st->st_mode = S_IFCHR; return 0; }
// int _isatty(int) { return 1; }
// int _lseek(int, int, int) { errno = ENOSYS; return -1; }
int _read(int, char*, int) { errno = ENOSYS; return 0; }
// int _unlink(const char*) { errno = ENOSYS; return -1; }
// int _open(const char*, int, int) { errno = ENOSYS; return -1; }

// procs
// int _kill(int, int) { errno = ENOSYS; return -1; }
// int _getpid(void) { return 1; }

// memory
// extern char _end;
// static char* heap_end;
// void* _sbrk(ptrdiff_t incr) { if (!heap_end) heap_end = &_end; char* p = heap_end; heap_end += incr; return p; }

// console write
// Пишите в UART/ITM здесь, если нужно видеть printf
int _write(int, const char* buf, int len) { (void)buf; return len; }
