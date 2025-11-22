#include <sys/errno.h>
#include <sys/stat.h>
#include "types.h"
#include "perepherial.h"
// files
// int _close(int) { errno = ENOSYS; return -1; }
// int _fstat(int, struct stat* st) { if (st) st->st_mode = S_IFCHR; return 0; }
// int _isatty(int) { return 1; }
// int _lseek(int, int, int) { errno = ENOSYS; return -1; }
int _read(int, cStringArray, int) { errno = ENOSYS; return 0; }
// int _unlink(cStringRO) { errno = ENOSYS; return -1; }
// int _open(cStringRO, int, int) { errno = ENOSYS; return -1; }

// procs
// int _kill(int, int) { errno = ENOSYS; return -1; }
// int _getpid(void) { return 1; }

// memory
// static cStringArray heap_end;
// void* _sbrk(ptrdiff_t incr) { if (!heap_end) heap_end = &_end; cStringArray p = heap_end; heap_end += incr; return p; }

// console write
// Пишите в UART/ITM здесь, если нужно видеть printf
#if 0
int _write(int, cStringRO buf, int len) { (void)buf; return len; }
#else
int _write(int file, const char *buf, int len) {
    // stdout(1) и stderr(2) отправляем в UART
    if (file == 1 || file == 2) {
        HAL_StatusTypeDef st = HAL_UART_Transmit(
            &huart1,
            (uint8_t *)buf,
            (uint16_t)len,
            HAL_MAX_DELAY
        );

        if (st == HAL_OK) {
            return len;
        } else {
            errno = EIO;
            return -1;
        }
    }

    // остальные файловые дескрипторы не поддерживаем
    errno = EBADF;
    return -1;
}
#endif