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

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main () {
    int fd, err, size;
    struct bzdg_user_slot *shmem;

    fd = open("/dev/bzdg", O_RDWR);

    if(!fd) {
        die("open");
    }

    size = ioctl(fd, SOCKET_WRAPPER_GET_SHMEM_SIZE);

    printf("size: %d\n", size);

    shmem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    printf("shmem : %p\n", shmem);

    ioctl(fd, SOCKET_WRAPPER_DBG);

    printf("shmem : %s\n", shmem[0].data);

    return 0;
}
