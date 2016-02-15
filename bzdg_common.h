#ifndef BZDG_COMMON_H
#define BZDG_COMMON_H

#define BZDG_OPTMEM_MAX 64
#define BZDG_BUFFER_SIZE 1500

/*
 * Name: bzdg_refs
 * Description: This struct holds references to shared memory
 * datastructure.
 */
struct bzdg_refs {
    struct msghdr *msg;
    struct sockaddr_in *addr;
    char *data;
    char *msgctl_area;
};

struct kernel_msghdr {
    void *msg_name;
    int msg_namelen;
    struct kernel_iov_iter {
        int type;
        size_t iov_offset;
        size_t count;
        struct iovec *iov;
        unsigned long nr_segs;
    } msg_iter;
    void *msg_control;
    size_t msg_controllen;
    unsigned int msg_flags;
    struct aiocb *iocb;
};

struct bzdg_user_slot {
    int is_available;
    struct kernel_msghdr msg;
    struct sockaddr_in addr;
    struct iovec vec;
    char msgctl_area[BZDG_OPTMEM_MAX];
    char data[BZDG_BUFFER_SIZE];
};

/* Macros needed in ioctl(2) */
#define BZDG_MAGIC 's'
#define BZDG_CONNECT _IOW(BZDG_MAGIC, 0, struct sockaddr)
#define BZDG_BIND _IOW(BZDG_MAGIC, 1, struct sockaddr)
#define BZDG_SEND _IO(BZDG_MAGIC, 2)
#define BZDG_GET_BATCH_NUM _IO(BZDG_MAGIC, 3)
#define BZDG_DBG _IO(BZDG_MAGIC, 4)

#endif
