#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "socket_wrapper_common.h"

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main () {
    int fd, err, size;
    struct socket_wrapper_slot *shmem;
    struct socket_wrapper_refs *refs;

    fd = open("/dev/socket_wrapper", O_RDWR);

    if(!fd) {
        die("open");
    }

    size = ioctl(fd, SOCKET_WRAPPER_GET_SHMEM_SIZE);

    printf("size: %d\n", size);

    shmem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    printf("shmem : %p\n", shmem);

    refs = malloc(sizeof(struct socket_wrapper_refs) * SOCKET_WRAPPER_BATCH_NUM);

    err = ioctl(fd, SOCKET_WRAPPER_GET_REFS, refs);

    if(err) {
        printf("ioctl(SOCKET_WRAPPER_GET_REFS) error: %d\n", err);
    }

    for(int i=0; i<SOCKET_WRAPPER_BATCH_NUM; i++) {
        printf("refs[i]: msg <%p>, addr <%p>, data <%p>, msgctl_area <%p>\n",
                refs[i].msg, refs[i].addr, refs[i].data, refs[i].msgctl_area);
    }

    return 0;
}
