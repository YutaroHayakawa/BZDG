#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "../bzdg_common.h"

#define PORTNUM 12345

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int fd, err;
    struct sockaddr_in src_addr;
    struct sockaddr_in dst_addr;

    fd = open("/dev/bzdg", 0, O_RDWR);

    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.58.20", &(src_addr.sin_addr));

    err = ioctl(fd, BZDG_BIND, (struct sockaddr *)&src_addr);

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(12345);
    inet_aton("192.168.58.1", &(dst_addr.sin_addr));

    err = ioctl(fd, BZDG_CONNECT, (struct sockaddr *)&dst_addr);

    err = ioctl(fd, BZDG_SEND);

    close(fd);

    return 0;
}
