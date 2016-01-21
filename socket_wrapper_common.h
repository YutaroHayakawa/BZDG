#ifndef SOCKET_WRAPPER_COMMON_H
#define SOCKET_WRAPPER_COMMON_H

#define SOCKET_WRAPPER_BATCH_NUM 4
#define SOCKET_WRAPPER_OPTMEM_MAX 64
#define SOCKET_WRAPPER_HEAD_ROOM 64
#define SOCKET_WRAPPER_BUFFER_SIZE 1564

/*
 * Name: socket_wrapper_refs
 * Description: This struct holds references to shared memory
 * datastructure.
 */
struct socket_wrapper_refs {
    struct msghdr *msg;
    struct sockaddr_in *addr;
    char *data;
    char *msgctl_area;
};

struct socket_wrapper_user_slot {
    int is_available;
    struct msghdr msg;
    struct sockaddr_in addr;
    struct iovec vec;
    char msgctl_area[SOCKET_WRAPPER_OPTMEM_MAX];
    char data[SOCKET_WRAPPER_BUFFER_SIZE];
    char shared_info_pad[320];
};

/* Macros needed in ioctl(2) */
#define SOCKET_WRAPPER_MAGIC 's'
#define SOCKET_WRAPPER_CONNECT _IOW(SOCKET_WRAPPER_MAGIC, 0, struct sockaddr)
#define SOCKET_WRAPPER_BIND _IOW(SOCKET_WRAPPER_MAGIC, 1, struct sockaddr)
#define SOCKET_WRAPPER_SEND _IO(SOCKET_WRAPPER_MAGIC, 2)
#define SOCKET_WRAPPER_GET_BATCH_NUM _IO(SOCKET_WRAPPER_MAGIC, 3)
#define SOCKET_WRAPPER_DBG _IO(SOCKET_WRAPPER_MAGIC, 4)

#endif
