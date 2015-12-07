#ifndef SOCKET_WRAPPER_COMMON_H
#define SOCKET_WRAPPER_COMMON_H

#include <sys/ioctl.h>

/* Macros needed in ioctl(2) */
#define SOCKET_WRAPPER_MAGIC 's'
#define SOCKET_WRAPPER_CONNECT _IOW(SOCKET_WRAPPER_MAGIC, 0, struct sockaddr)
#define SOCKET_WRAPPER_SEND _IO(SOCKET_WRAPPER_MAGIC, 1)

#endif
