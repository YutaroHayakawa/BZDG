#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "socket_wrapper_common.h"

typedef struct socket_wrapper {
    int fd;
    int batch_num;
    struct socket_wrapper_user_slot *shmem;
}SOCKET_WRAPPER;


SOCKET_WRAPPER* init_socket_wrapper(void) {
    SOCKET_WRAPPER *ret;

    ret = malloc(sizeof(SOCKET_WRAPPER));
    if(!ret) {
        return NULL;
    }

    ret->fd = open("/dev/socket_wrapper", O_RDWR);
    if(!ret->fd) {
        free(ret);
        return NULL;
    }

    ret->batch_num = ioctl(ret->fd, SOCKET_WRAPPER_GET_BATCH_NUM);
    if(!ret->batch_num) {
        close(ret->fd);
        free(ret);
        return NULL;
    }

    ret->shmem = mmap(0, ret->batch_num * sizeof(struct socket_wrapper_user_slot), PROT_READ|PROT_WRITE, MAP_SHARED, ret->fd, 0);
    if(ret->shmem == MAP_FAILED) {
        close(ret->fd);
        free(ret);
        return NULL;
    }

    return ret;
}

int socket_wrapper_get_batch_num(SOCKET_WRAPPER *sw) {
    return sw->batch_num;
}

struct msghdr *socket_wrapper_get_tx_msghdr(SOCKET_WRAPPER *sw, int index) {
    return &(sw->shmem[index].msg);
}

struct sockaddr_in *socket_wrapper_get_tx_sockaddr_in(SOCKET_WRAPPER *sw, int index) {
    return &(sw->shmem[index].addr);
}

struct iovec *socket_wrapper_get_tx_iovec(SOCKET_WRAPPER *sw, int index) {
    return &(sw->shmem[index].vec);
}

/* TODO SOCKET_WRAPPER_HEAD_ROOM should not be a macro.*/
char *socket_wrapper_get_tx_data_buffer(SOCKET_WRAPPER *sw, int index) {
    return sw->shmem[index].data + SOCKET_WRAPPER_HEAD_ROOM;
}

char *socket_wrapper_get_tx_msgctl_area(SOCKET_WRAPPER *sw, int index) {
    return sw->shmem[index].msgctl_area;
}

int socket_wrapper_send(SOCKET_WRAPPER *sw) {
    return ioctl(sw->fd, SOCKET_WRAPPER_SEND);
}

int socket_wrapper_bind(SOCKET_WRAPPER *sw, struct sockaddr *addr, socklen_t addrlen) {
    return ioctl(sw->fd, SOCKET_WRAPPER_BIND, addr);
}

int socket_wrapper_connect(SOCKET_WRAPPER *sw, struct sockaddr *addr, socklen_t addrlen) {
    return ioctl(sw->fd, SOCKET_WRAPPER_CONNECT, addr);
}

void socket_wrapper_buffer_ready(SOCKET_WRAPPER *sw, int index) {
    sw->shmem[index].is_available = 1;
}
