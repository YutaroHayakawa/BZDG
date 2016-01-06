#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/task_work.h>
#include <linux/tick.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/cpu.h>
#include <linux/uio.h>

#include <linux/list.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/inet.h>

#include <asm/desc.h>
#include <asm/uaccess.h>

#include <uapi/linux/in.h> 

#include <net/sock.h>

#define SOCKET_WRAPPER_OPTMEM_MAX 100
#define SOCKET_WRAPPER_BUFFER_SIZE 1450
#define SOCKET_WRAPPER_HEAD_ROOM 100

struct socket_wrapper_refs {
    struct msghdr *msg;
    struct sockaddr_in *addr;
    char *data;
    char *msgctl_area;
};

struct build_skb_struct {
    char data[SOCKET_WRAPPER_BUFFER_SIZE];
    struct skb_shared_info shinfo;
};

struct socket_wrapper_slot {
    struct msghdr msg;
    struct sockaddr_in addr;
    struct build_skb_struct bskb;
    char msgctl_area[SOCKET_WRAPPER_OPTMEM_MAX];
};

struct kvec *socket_wrapper_create_kvec_from_slot(struct socket_wrapper_slot *slot)
{
    struct kvec *ret = NULL;

    if(!slot) {
        return NULL;
    }

    ret = kmalloc(sizeof(struct kvec), GFP_KERNEL);

    if(!ret) {
        return NULL;
    }

    ret->iov_base = &(slot->bskb.data[SOCKET_WRAPPER_HEAD_ROOM]);
    ret->iov_len = 0;

    return ret;
}

void socket_wrapper_release_kvec(struct kvec *vec)
{
    if(!vec) {
        return;
    }

    kfree(vec);
}

struct socket_wrapper_refs *create_refs_from_slot(struct socket_wrapper_slot *slot)
{
    struct socket_wrapper_refs *ret = NULL;

    if(!slot) {
        return NULL;
    }

    ret = kmalloc(sizeof(struct socket_wrapper_refs), GFP_KERNEL);

    if(!ret) {
        return NULL;
    }

    ret->msg = &(slot->msg);
    ret->addr = &(slot->addr);
    ret->data = &(slot->bskb.data[SOCKET_WRAPPER_HEAD_ROOM]);
    ret->msgctl_area = slot->msgctl_area;

    return ret;
}

void socket_wrapper_release_refs(struct socket_wrapper_refs *refs)
{
    if(!refs) {
        return;
    }

    kfree(refs);
}

struct sk_buff *build_skb_from_slot(struct socket_wrapper_slot *slot)
{
    struct sk_buff *ret = NULL;

    if(!slot) {
        return NULL;
    }

    ret = build_skb(&(slot->bskb), sizeof(struct build_skb_struct));

    if(!ret) {
        return NULL;
    }

    ret->head = ret->head + SOCKET_WRAPPER_HEAD_ROOM;
    ret->data = ret->data + SOCKET_WRAPPER_HEAD_ROOM;
    skb_reset_tail_pointer(ret);

    return ret;
}

void socket_wrapper_release_skb(struct sk_buff *skb) {
    if(!skb) {
        return;
    }

    kfree_skb(skb);
}
