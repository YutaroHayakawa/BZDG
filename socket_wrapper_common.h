#ifndef SOCKET_WRAPPER_COMMON_H
#define SOCKET_WRAPPER_COMMON_H

#define SOCKET_WRAPPER_BATCH_NUM 4
#define SOCKET_WRAPPER_BUFFER_SIZE 1450
#define SOCKET_WRAPPER_OPTMEM_MAX 100

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
    struct msghdr msg;
    struct sockaddr_in addr;
    char data[SOCKET_WRAPPER_BUFFER_SIZE];
    char shared_info_pad[320];
    char msgctl_area[SOCKET_WRAPPER_OPTMEM_MAX];
};

/* Macros needed in ioctl(2) */
#define SOCKET_WRAPPER_MAGIC 's'
#define SOCKET_WRAPPER_CONNECT _IOW(SOCKET_WRAPPER_MAGIC, 0, struct sockaddr)
#define SOCKET_WRAPPER_BIND _IOW(SOCKET_WRAPPER_MAGIC, 1, struct sockaddr)
#define SOCKET_WRAPPER_SEND _IO(SOCKET_WRAPPER_MAGIC, 2)
#define SOCKET_WRAPPER_GET_REFS _IOR(SOCKET_WRAPPER_MAGIC, 3, struct socket_wrapper_refs)
#define SOCKET_WRAPPER_GET_SHMEM_SIZE _IO(SOCKET_WRAPPER_MAGIC, 4)
#define SOCKET_WRAPPER_DBG _IO(SOCKET_WRAPPER_MAGIC, 5)

#endif
