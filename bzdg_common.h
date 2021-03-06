#ifndef BZDG_COMMON_H
#define BZDG_COMMON_H

#define BZDG_BATCH_NUM 4
#define BZDG_OPTMEM_MAX 64
#define BZDG_HEAD_ROOM 200
#define BZDG_BUFFER_SIZE 1084

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

struct bzdg_user_slot {
    int is_available;
    struct msghdr msg;
    struct sockaddr_in addr;
    struct iovec vec;
    char msgctl_area[BZDG_OPTMEM_MAX];
    char data[BZDG_BUFFER_SIZE];
    char shared_info_pad[320];
};

/* Macros needed in ioctl(2) */
#define BZDG_MAGIC 's'
#define BZDG_CONNECT _IOW(BZDG_MAGIC, 0, struct sockaddr)
#define BZDG_BIND _IOW(BZDG_MAGIC, 1, struct sockaddr)
#define BZDG_SEND _IO(BZDG_MAGIC, 2)
#define BZDG_GET_BATCH_NUM _IO(BZDG_MAGIC, 3)
#define BZDG_DBG _IO(BZDG_MAGIC, 4)

#endif
