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
    struct kernel_msghdr *msg = NULL;
    struct sockaddr_in *addr = NULL;
    struct iovec *vec = NULL;
    char *data = NULL;
    BZDG *bzdg = NULL;

    /* Parameters */
    int packet_size = 0;
    int loop_num = 0;
    int batch_num = 0;

    clock_t start = 0;
    clock_t end = 0;

    if(argc != 2) {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }

    packet_size = atoi(argv[1]);

    bzdg = init_bzdg();

    if(!bzdg) {
        die("init_bzdg");
    }

    /* bind address to inner socket */
    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.131.90", (struct in_addr *)&(src_addr.sin_addr));
    err = bzdg_bind(bzdg, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

    if(err) {
	    exit(EXIT_FAILURE);
    }

    batch_num = bzdg_get_batch_num(bzdg);
    loop_num = (1024*1024*1024)/(packet_size * batch_num);

    start = clock();
    for(i=0; i<loop_num; i++) {
        for(j=0; j<batch_num; j++) {
            addr = bzdg_get_tx_sockaddr_in(bzdg, j);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(12345);
            inet_aton("192.168.131.1", &(addr->sin_addr));

            msg = bzdg_get_tx_msghdr(bzdg, j);
            msg->msg_name = addr;
            msg->msg_namelen = sizeof(struct sockaddr_in);
            msg->msg_control = NULL;
            msg->msg_flags = 0;

            data = bzdg_get_tx_data_buffer(bzdg, j);
            memset(data, 0, BZDG_BUFFER_SIZE);
            memset(data, 'A', packet_size);

            vec = bzdg_get_tx_iovec(bzdg, j);
            vec->iov_base = data;
            vec->iov_len = packet_size;

            msg->msg_iter.iov = vec;

            bzdg_buffer_ready(bzdg, j);
        }
        bzdg_send(bzdg);
    }
    end = clock();

    free_bzdg(bzdg);

    printf("%d,%.2f,%ld\n", packet_size, (double)(end-start), CLOCKS_PER_SEC);

    return 0;
}
