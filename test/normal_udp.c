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
#include <errno.h>

int main () {
    int i, j, err, fd;
    struct sockaddr_in src_addr;
    struct msghdr msg;
    struct sockaddr_in addr;
    struct iovec vec;
    char data[1024];

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* bind address to inner socket */
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
	  err = inet_aton("192.168.130.80", &(src_addr.sin_addr));

	  if(!err) {
		  printf("inet_aton 1 %d\n", err);
		  exit(EXIT_FAILURE);
	  }

    err = bind(fd, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

    if(err) {
	    exit(EXIT_FAILURE);
    }

    for(i=0; i<10000; i++) {
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(12345);
	    inet_aton("192.168.130.20", &(src_addr.sin_addr));

	    memset(data, 0, 1024);
	    memset(data, 'A', 1024);

	    err = sendto(fd, data, 1024, 0, (struct sockaddr *)&addr, sizeof(addr));

	    if(err) {
		    perror("sendmsg");
		    printf("[%d] : %d\n", i, errno);
	    }
    }


    return 0;
}
