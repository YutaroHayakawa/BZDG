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

#include "../socket_wrapper_common.h"

#define PORTNUM 12345

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int fd, err;
    struct sockaddr_in addr1;
    struct sockaddr_in addr2;

    fd = open("/dev/socket_wrapper", 0, O_RDWR);

    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(12345);
    inet_aton("192.168.56.70", &(addr1.sin_addr));

    err = ioctl(fd, SOCKET_WRAPPER_BIND, &addr1);

    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(12345);
    inet_aton("192.168.56.102", &(addr2.sin_addr));

    err = ioctl(fd, SOCKET_WRAPPER_CONNECT, &addr2);

    err = ioctl(fd, SOCKET_WRAPPER_SEND);

    printf("%s\n", strerror(err));

    printf("Test OK!\n");
    return 0;
}




