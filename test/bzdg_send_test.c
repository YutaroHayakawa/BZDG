#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "bzdg_common.h"

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main () {
    int i, j, fd, err, size, batch_num;
    struct bzdg_user_slot *shmem;
    struct sockaddr_in src_addr;
    struct sockaddr_in dst_addr;

    fd = open("/dev/bzdg", O_RDWR);
    if(!fd) {
        die("open");
    }

    /* bind address to inner socket */
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.58.20", (struct in_addr *)&(src_addr.sin_addr));

    err = ioctl(fd, BZDG_BIND, (struct sockaddr *)&src_addr);

    /* Get shared memory */
    batch_num = ioctl(fd, BZDG_GET_BATCH_NUM);
    size = batch_num * sizeof(struct bzdg_user_slot);
    printf("size: %d\n", size);
    shmem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    memset(shmem, 0, size);
    printf("shmem : %p\n", shmem);

    for(j=0; j<10; j++) {
        for(i=0; i<batch_num; i++) {
            shmem[i].addr.sin_family = AF_INET;
            shmem[i].addr.sin_port = htons(12345);
            inet_aton("192.168.58.1", &(shmem[i].addr.sin_addr));

            shmem[i].msg.msg_name = &(shmem[i].addr);
            shmem[i].msg.msg_namelen = sizeof(struct sockaddr_in);
            shmem[i].msg.msg_control = NULL;
            shmem[i].msg.msg_flags = 0;

            shmem[i].vec.iov_base = shmem[i].data + BZDG_HEAD_ROOM;
            shmem[i].vec.iov_len = strlen("hogehoge");

            memset(shmem[i].data + BZDG_HEAD_ROOM, 0, BZDG_BUFFER_SIZE - BZDG_HEAD_ROOM);
            strcpy(shmem[i].data + BZDG_HEAD_ROOM, "hogehoge");

            shmem[i].is_available = 1;
        }
        ioctl(fd, BZDG_SEND);
    }

    return 0;
}
