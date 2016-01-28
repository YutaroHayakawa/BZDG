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
#include "bzdg_uapi.h"

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main () {
    int i, j, err;
    struct sockaddr_in src_addr;
    struct msghdr *msg = NULL;
    struct sockaddr_in *addr;
    struct iovec *vec;
    char *data;
    BZDG *bzdg;

    bzdg = init_bzdg();

    if(!bzdg) {
        perror("init_bzdg()");
        exit(EXIT_FAILURE);
    }

    /* bind address to inner socket */
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.130.80", (struct in_addr *)&(src_addr.sin_addr));
    err = bzdg_bind(bzdg, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

    if(err) {
	    exit(EXIT_FAILURE);
    }

    for(j=0; j<100000; j++) {
        for(i=0; i<bzdg_get_batch_num(bzdg); i++) {
            addr = bzdg_get_tx_sockaddr_in(bzdg, i);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(12345);
            inet_aton("192.168.130.100", &(addr->sin_addr));

            msg = bzdg_get_tx_msghdr(bzdg, i);
            msg->msg_name = addr;
            msg->msg_namelen = sizeof(struct sockaddr_in);
            msg->msg_control = NULL;
            msg->msg_flags = 0;

            data = bzdg_get_tx_data_buffer(bzdg, i);
            memset(data, 0, BZDG_BUFFER_SIZE - BZDG_HEAD_ROOM);
            memset(data, 'A', 1024);

            vec = bzdg_get_tx_iovec(bzdg, i);
            vec->iov_base = data;
            vec->iov_len = 1024;

            msg->msg_iov = vec;
            msg->msg_iovlen = vec->iov_len;

            bzdg_buffer_ready(bzdg, i);
        }
        bzdg_send(bzdg);
    }

    free_bzdg(bzdg);

    return 0;
}
