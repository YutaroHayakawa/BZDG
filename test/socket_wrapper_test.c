#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../socket_wrapper_common.h"

#define PORTNUM 12345

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int fd, err;
    struct sockaddr_in addr;

    fd = open("/dev/socket_wrapper", 0, O_RDWR);

    if(fd < 0) {
        die("open()");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORTNUM);
    err = inet_aton("192.168.56.100", &(addr.sin_addr));

    if(err == 0) {
        printf("inet_aton() failed\n");
        exit(EXIT_FAILURE);
    }

    err = ioctl(fd, SOCKET_WRAPPER_CONNECT, &addr);

    if(err < 0) {
        die("ioctl(SOCKET_WRAPPER_CONNECT)");
    }

    printf("Test succeeded\n");

    return 0;
}




