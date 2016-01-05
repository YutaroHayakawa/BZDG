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

#include "kmod.h"
#include "socket_wrapper_common.h"

struct socket_wrapper_info {
    void *shmem;
    struct socket *sock;
};

/*
struct build_skb_struct {
    void *udata;
    struct skb_shared_info shinfo;
};
*/

static int socket_wrapper_open(struct inode *inode, struct file *file)
{
    int err;
    struct socket *sock;
    struct socket_wrapper_info *info;

    file->private_data = (struct socket_wrapper_info *)kmalloc(sizeof(struct socket_wrapper_info), GFP_KERNEL);
    info = file->private_data;

    /* Make UDP socket */
    err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);

    if(err) {
        KMODD("Socket create failed");
        return -EINVAL;
    }

    /* Keep socket object reference in file object.*/
    info->sock = sock;
    info->shmem = NULL;

    return 0;
}

static int socket_wrapper_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int error;
    u64 pa;
    void *field;
    size_t size;
    unsigned int order;
    struct socket_wrapper_info *info;

    size = vma->vm_end - vma->vm_start;
    order = get_order(size);
    field = (void *) __get_free_pages(GFP_ATOMIC | __GFP_ZERO, order);
    if (!field) {
        KMODD("__get_free_pages failed");
        return -EINVAL;
    }
    if ((vma->vm_start & ~PAGE_MASK) || (vma->vm_end & ~PAGE_MASK)) {
        KMODD("vm_start = %lx vm_end = %lx", vma->vm_start, vma->vm_end);
        return -EINVAL;
    }

    pa = virt_to_phys(field);
    error = remap_pfn_range(vma, vma->vm_start, pa >> PAGE_SHIFT, size, vma->vm_page_prot);
    if (error) {
        KMODD("remap_pfn_range error");
        return error;
    }

    info->shmem = (void *)pa;

    return 0;
}

struct build_skb_struct {
    char data[1450];
    struct skb_shared_info shinfo;
};

long socket_wrapper_ioctl(struct file *filp, u_int cmd, u_long data)
{
    int err;
    struct sockaddr_in addr;
    struct sk_buff *skb = NULL;
    struct socket *sock = NULL;
    struct socket_wrapper_info *info = NULL;
    struct kvec vec;
    struct msghdr msg;
    struct build_skb_struct *bskb;
    unsigned int head_room;

    info = filp->private_data;

    if(!info) {
        KMODD("There is no private in file structure.\n");
        return -EINVAL;
    }

    switch(cmd) {
        /* 
         * SOCKET_WRAPPER_CONNECT
         * 1. Copy IP address and Port number data from user space.
         *    Data must be passed by struct sockaddr_in format.
         *
         * 2. Call connect().
         *
         */
        case SOCKET_WRAPPER_CONNECT:
            err = copy_from_user(&addr, (struct sockaddr*)data, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("Copy of sockaddr data from user space failed.\n");
                return -EINVAL;
            }

            err = kernel_connect(info->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr), 0);

            if(err != 0) {
                KMODD("kernel_connect() failed.\n");
                return -EINVAL;
            }

            break;

        case SOCKET_WRAPPER_BIND:
            err = copy_from_user(&addr, (struct sockaddr*)data, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("Copy of sockaddr data from user space failed.\n");
                return -EINVAL;
            }

            err = kernel_bind(info->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("kernel_bind() failed.\n");
                return -EINVAL;
            }

            break; 

        case SOCKET_WRAPPER_SEND:
            bskb = kmalloc(sizeof(struct build_skb_struct), GFP_KERNEL);
            memset(bskb, 0, sizeof(struct build_skb_struct));

            skb = build_skb(bskb, sizeof(struct build_skb_struct));

            if(skb == NULL) {
                return -EINVAL;
            }

            head_room = 100;

            printk("head : %p\ndata : %p\ntail : %d\nend : %d\n", skb->head, skb->data, skb->tail, skb->end);

            skb->head = skb->head + head_room;
            skb->data = skb->data + head_room;
            skb_reset_tail_pointer(skb);
            /* We don't need to change end pointer. */

            printk("head : %p\ndata : %p\ntail : %d\nend : %d\n", skb->head, skb->data, skb->tail, skb->end);
            memset(&vec, 0, sizeof(vec));
            vec.iov_base = bskb->data + head_room;
            printk("vec.iov_base: %p\n", vec.iov_base);
            strcpy(bskb->data + head_room, "hogehoge");
            vec.iov_len = strlen(bskb->data + head_room);

            memset(&msg, 0, sizeof(msg));
            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            msg.msg_control = NULL;
            msg.msg_flags = 0;

            sock = info->sock;
            sock->sk->sk_user_data = skb;

            err = kernel_sendmsg(sock, &msg, &vec, 1, strlen(bskb->data + head_room));
            printk("kernel_sendmsg: %d\n", err);
            return err;

        default:
            KMODD("Invalid argument.\n");
            return -EINVAL;
    }

    return 0;
}

static int socket_wrapper_release(struct inode *inode, struct file *filp)
{
    struct socket_wrapper_info *info = filp->private_data;
    sock_release(info->sock);
    kfree(filp->private_data);
    return 0;
}

static struct file_operations socket_wrapper_fops = {
    .owner = THIS_MODULE,
    .open = socket_wrapper_open,
    .mmap = socket_wrapper_mmap,
    .unlocked_ioctl = socket_wrapper_ioctl,
    .release = socket_wrapper_release,
};

struct miscdevice socket_wrapper_cdevsw = {
    MISC_DYNAMIC_MINOR,
    "socket_wrapper",
    &socket_wrapper_fops,
};

static int __init socket_wrapper_module_init(void)
{
    misc_register(&socket_wrapper_cdevsw);
    KMODD("socket_wrapper loaded");
    return 0;
}

static void __exit socket_wrapper_module_exit(void)
{
    misc_deregister(&socket_wrapper_cdevsw);
    KMODD("KMOD Unloaded");
    return;
}
/* -------------------------------------------------------------------- */

module_init(socket_wrapper_module_init);
module_exit(socket_wrapper_module_exit);

MODULE_AUTHOR("Yutaro Hayakawa");
MODULE_DESCRIPTION("BZDG");
MODULE_LICENSE("Dual BSD/GPL");
