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

struct SOCKET_WRAPPER {
    int current;
    int fd;
    int batch_num;
    struct socket_wrapper_user_slot *shmem;
    struct msghdr *msgs;
    struct sockaddr_in *addrs;
    struct char *datas;
    struct char *msgctl_areas;
};

void socket_wrapper_die(const char* msg) {
    perror("%s\n", msg);
    exit(EXIT_FAILURE);
}

struct SOCKET_WRAPPER* init_socket_wrapper {
    struct SOCKET_WRAPPER *ret;

    ret = malloc(sizeof(SOCKET_WRAPPER));
    if(!ret) {
        socket_wrapper_die("malloc of struct SOCKET_WRAPPER");
    }

    ret->fd = open("/dev/socket_wrapper", O_RDWR);
    if(!ret->fd) {
        socket_wrapper_die("open() of /dev/socket_wrapper");
    }

    ret->batch_num = ioctl(fd, SOCKET_WRAPPER_GET_BATCH_NUM);
    if(!ret->fd) {
        socket_wrapper_die("ioctl(SOCKET_WRAPPER_GET_BATCH_NUM failed.")
    }

    ret->msgs = malloc(sizeof(struct msghdr *) * ret->batch_num);
    if(!ret->msgs) {
        socket_wrapper_die("malloc of msghdrs");
    }

    ret->addrs = malloc(sizeof(struct sockaddr_in *) * ret->batch_num);
    if(!ret->addrs) {
        socket_wrapper_die("malloc of addrs");
    }

    ret->msgctl_areas = malloc(
