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
#include "socket_wrapper_common.h"
#include "socket_wrapper_uapi.h"

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
    SOCKET_WRAPPER *sw;

    sw = init_socket_wrapper();

    if(!sw) {
        perror("init_socket_wrapper()");
        exit(EXIT_FAILURE);
    }

    /* bind address to inner socket */
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(12345);
    inet_aton("192.168.58.20", (struct in_addr *)&(src_addr.sin_addr));
    socket_wrapper_bind(sw, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

    for(j=0; j<10; j++) {
        for(i=0; i<socket_wrapper_get_batch_num(sw); i++) {
            addr = socket_wrapper_get_tx_sockaddr_in(sw, i);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(12345);
            inet_aton("192.168.58.1", &(addr->sin_addr));

            msg = socket_wrapper_get_tx_msghdr(sw, i);
            msg->msg_name = addr;
            msg->msg_namelen = sizeof(struct sockaddr_in);
            msg->msg_control = NULL;
            msg->msg_flags = 0;

            vec = socket_wrapper_get_tx_iovec(sw, i);
            vec->iov_base = data;
            vec->iov_len = strlen("hogehoge");

            data = socket_wrapper_get_tx_data_buffer(sw, i);
            memset(data, 0, SOCKET_WRAPPER_BUFFER_SIZE - SOCKET_WRAPPER_HEAD_ROOM);
            strcpy(data, "hogehoge");

            socket_wrapper_buffer_ready(sw, i);
        }
        socket_wrapper_send(sw);
    }


    return 0;
}
