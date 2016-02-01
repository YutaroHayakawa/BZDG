#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
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

int main (int argc, char** argv) {
    int i, j, err;
    struct sockaddr_in src_addr;
    struct sockaddr_in dst_addr;
    struct kernel_msghdr *msg = NULL;
    struct sockaddr_in *addr = NULL;
    struct iovec *vec = NULL;
    char *data = NULL;
    BZDG *bzdg = NULL;

    /* Parameters */
    int packet_size;
    int loop_num;
    int batch_num;

    clock_t start, end;

    if(argc < 2) {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }

    packet_size = atoi(argv[1]);

    bzdg = init_bzdg();

    if(!bzdg) {
        die("init_bzdg");
    }

    /* bind address to inner socket */
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.130.80", (struct in_addr *)&(src_addr.sin_addr));
    err = bzdg_bind(bzdg, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

    if(err) {
	    exit(EXIT_FAILURE);
    }

    batch_num = bzdg_get_batch_num(bzdg);
    loop_num = (1024*1024*1024)/(packet_size * batch_num);

    start = clock();
    for(j=0; j<10; j++) {
        for(i=0; i<batch_num; i++) {
            addr = bzdg_get_tx_sockaddr_in(bzdg, i);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(12345);
            inet_aton("192.168.130.90", &(addr->sin_addr));

            msg = bzdg_get_tx_msghdr(bzdg, i);
            msg->msg_name = addr;
            msg->msg_namelen = sizeof(struct sockaddr_in);
            msg->msg_control = NULL;
            msg->msg_flags = 0;

            data = bzdg_get_tx_data_buffer(bzdg, i);
            memset(data, 0, BZDG_BUFFER_SIZE - BZDG_HEAD_ROOM);
            memset(data, 'A', packet_size);

            vec = bzdg_get_tx_iovec(bzdg, i);
            vec->iov_base = data;
            vec->iov_len = packet_size;

            msg->msg_iter.iov = vec;

            bzdg_buffer_ready(bzdg, i);
        }
        bzdg_send(bzdg);
    }
    end = clock();

    free_bzdg(bzdg);

    printf("%d,%.2f,%ld\n", packet_size, (double)(end-start), CLOCKS_PER_SEC);

    return 0;
}
