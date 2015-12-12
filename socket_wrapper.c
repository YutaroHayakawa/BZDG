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

#define BZDG_BATCH_NUM 7

static int socket_wrapper_open(struct inode *inode, struct file *file)
{
    int err;
    struct socket *sock;

    /* Make UDP socket */
    err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);

    if(err) {
        KMODD("Socket create failed");
        return -EINVAL;
    }

    /* Keep socket object reference in file object.*/
    file->private_data = (struct socket *)sock;

    return 0;
}

long socket_wrapper_ioctl(struct file *filp, u_int cmd, u_long data)
{
    int err;
    struct sockaddr_in addr;
    struct sk_buff *skb = NULL;
    struct socket *sock = NULL;
    struct kvec vec;
    struct msghdr msg;
    unsigned char *dat = NULL;

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

            err = kernel_connect((struct socket *)filp->private_data, (struct sockaddr *)&addr, sizeof(struct sockaddr), 0);

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

            err = kernel_bind((struct socket *)filp->private_data, (struct sockaddr *)&addr, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("kernel_bind() failed.\n");
                return -EINVAL;
            }

            break; 

        case SOCKET_WRAPPER_SEND:
            skb = alloc_skb(1320, GFP_KERNEL);
            skb_reserve(skb, 100);
            dat = skb_put(skb, 500);
            memset(dat, 'A', 500);

            memset(&vec, 0, sizeof(vec));
            vec.iov_base = dat;
            vec.iov_len = 500;

            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            msg.msg_control = NULL;
            msg.msg_flags = 0;
            sock = (struct socket *)filp->private_data;
            sock->sk->sk_user_data = skb;

            err = kernel_sendmsg(sock, &msg, &vec, 1, 0);
            printk("%s: %d\n", __func__, err);
            return err;

        default:
            KMODD("Invalid argument.\n");
            return -EINVAL;
    }

    return 0;
}

static int socket_wrapper_release(struct inode *inode, struct file *filp)
{
    sock_release((struct socket *)filp->private_data);
    return 0;
}

static struct file_operations socket_wrapper_fops = {
    .owner = THIS_MODULE,
    .open = socket_wrapper_open,
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
