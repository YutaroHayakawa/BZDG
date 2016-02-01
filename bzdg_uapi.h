#ifndef BZDG_UAPI_H
#define BZDG_UAPI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "bzdg_common.h"

typedef struct bzdg {
    int fd;
    int batch_num;
    struct bzdg_user_slot *shmem;
}BZDG;


BZDG* init_bzdg(void) {
    BZDG *ret;

    ret = malloc(sizeof(BZDG));
    if(!ret) {
        return NULL;
    }

    ret->fd = open("/dev/bzdg", O_RDWR);
    if(!ret->fd) {
        free(ret);
        return NULL;
    }

    ret->batch_num = ioctl(ret->fd, BZDG_GET_BATCH_NUM);
    if(!ret->batch_num) {
        close(ret->fd);
        free(ret);
        return NULL;
    }

    ret->shmem = mmap(0, ret->batch_num * sizeof(struct bzdg_user_slot), PROT_READ|PROT_WRITE, MAP_SHARED, ret->fd, 0);
    if(ret->shmem == MAP_FAILED) {
        close(ret->fd);
        free(ret);
        return NULL;
    }

    return ret;
}

void free_bzdg(BZDG *bzdg) {
    close(bzdg->fd);
    free(bzdg);
}

int bzdg_get_batch_num(BZDG *bzdg) {
    return bzdg->batch_num;
}

struct kernel_msghdr *bzdg_get_tx_msghdr(BZDG *bzdg, int index) {
    return &(bzdg->shmem[index].msg);
}

struct sockaddr_in *bzdg_get_tx_sockaddr_in(BZDG *bzdg, int index) {
    return &(bzdg->shmem[index].addr);
}

struct iovec *bzdg_get_tx_iovec(BZDG *bzdg, int index) {
    return &(bzdg->shmem[index].vec);
}

/* TODO BZDG_HEAD_ROOM should not be a macro.*/
char *bzdg_get_tx_data_buffer(BZDG *bzdg, int index) {
    return bzdg->shmem[index].data + BZDG_HEAD_ROOM;
}

char *bzdg_get_tx_msgctl_area(BZDG *bzdg, int index) {
    return bzdg->shmem[index].msgctl_area;
}

int bzdg_send(BZDG *bzdg) {
    return ioctl(bzdg->fd, BZDG_SEND);
}

int bzdg_bind(BZDG *bzdg, struct sockaddr *addr, socklen_t addrlen) {
    return ioctl(bzdg->fd, BZDG_BIND, addr);
}

int bzdg_connect(BZDG *bzdg, struct sockaddr *addr, socklen_t addrlen) {
    return ioctl(bzdg->fd, BZDG_CONNECT, addr);
}

void bzdg_buffer_ready(BZDG *bzdg, int index) {
    bzdg->shmem[index].is_available = 1;
}

#endif
