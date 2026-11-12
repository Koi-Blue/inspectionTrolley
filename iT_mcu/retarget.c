#include "board_uart.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

int _write(int fd, char *ptr, int len)
{
    if (ptr == NULL || len <= 0) {
        return 0;
    }
    if (fd != 1 && fd != 2) {
        errno = EBADF;
        return -1;
    }
    if (Uart_Write(&g_board_uart, (const uint8_t *)ptr, (size_t)len) != (size_t)len) {
        errno = EIO;
        return -1;
    }
    return len;
}

int _read(int fd, char *ptr, int len)
{
    size_t got;

    if (ptr == NULL || len <= 0) {
        return 0;
    }
    if (fd != 0) {
        errno = EBADF;
        return -1;
    }
    got = Uart_Read(&g_board_uart, (uint8_t *)ptr, (size_t)len);
    if (got == 0u) {
        errno = EIO;
        return -1;
    }
    return (int)got;
}
