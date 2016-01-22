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
#include "bzdg_common.h"

/*
 * Name: build_skb_struct;
 * Description: This memory layout is needed in build_skb function.
 */
struct build_skb_struct {
    char data[BZDG_BUFFER_SIZE];
    struct skb_shared_info shinfo;
};

/*
 * Name: bzdg_slot
 * Description:
 *  This struct expresses the layout of 'slot'
 *  on shared memory. 'slot' contains data structure
 *  needed to call sendmsg().
 *
 *  bzdg's shared memory will be treated
 *  as an array of this 'slot'
 */
struct bzdg_slot {
    int is_available;
    struct msghdr msg;
    struct sockaddr_in addr;
    struct kvec vec;
    char msgctl_area[BZDG_OPTMEM_MAX];
    struct build_skb_struct bskb;
};


/*
 * Name: bzdg_info
 * Description: This struct manages important objects for bzdg.
 */
struct bzdg_info {
    struct bzdg_slot *shmem;
    struct sk_buff *skbs[BZDG_BATCH_NUM];
    struct socket *sock;
};

/*
 * Name: bzdg_open
 * Description:
 *  This function makes instance of bzdg_info
 *  and socket.
 * Note:
 *  Following members of bzdg_info is NULL after
 *  calling this function.
 *  - refs
 *  - shmem
 *  please pay attention not to do NULL reference.
 */
static int bzdg_open(struct inode *inode, struct file *file)
{
    int err;
    struct socket *sock;
    struct bzdg_info *info;

    file->private_data = (struct bzdg_info *)kmalloc(sizeof(struct bzdg_info), GFP_KERNEL);
    info = file->private_data;

    /* Make UDP socket */
    err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);

    if(err) {
        KMODD("Socket create failed");
        return -EINVAL;
    }

    /* Keep socket object reference in file object.*/
    info->sock = sock;
    memset(info->skbs, 0, sizeof(struct sk_buff *) * BZDG_BATCH_NUM);
    info->shmem = NULL;

    return 0;
}

void bzdg_build_skbs(struct bzdg_info *info, int num) {
    int i;

    for(i=0; i<num; i++) {
        info->skbs[i] = build_skb(&(info->shmem[i].bskb), sizeof(info->shmem[i].bskb));
        KMODD("info->skbs[%d] <%p>", i, info->skbs[i]);
    }
}

/*
 * Name: bzdg_mmap
 * Description:
 *  This function create user kernel shared memory
 *  and also set reference to 'slot' mapped in shared memory
 *  to bzdg_info.
 */
static int bzdg_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int err;
    u64 pa;
    void *field;
    size_t size;
    unsigned int order;
    struct bzdg_info *info = filp->private_data;

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
    err = remap_pfn_range(vma, vma->vm_start, pa >> PAGE_SHIFT, size, vma->vm_page_prot);
    if (err) {
        KMODD("remap_pfn_range error");
        return err;
    }

    info->shmem = (struct bzdg_slot *)field;

    /*
     * Execute build_skb and get reference.
     */
    bzdg_build_skbs(info, BZDG_BATCH_NUM);

    return 0;
}

void bzdg_reset_skb(struct bzdg_info *info, struct sk_buff *skb, int index) {
    skb_orphan(skb);
    skb->len = 0;
    skb->head = info->shmem[index].bskb.data+BZDG_HEAD_ROOM;
    skb->data = info->shmem[index].bskb.data+BZDG_HEAD_ROOM;
    skb->tail = 0;
}

void dump_slot(struct bzdg_slot slot) {
    KMODD("slot <%p>", &slot);
    KMODD("data %s", slot.bskb.data);
    /*
    KMODD("msg : msg_name <%p>, msg_namelen %d, msg_iov <%p>, msg_iovlen %d", slot->msg.msg_name, slot->msg.msg_namelen, slot->msg.msg_iov, slot->msg.msg_iovlen);
    KMODD("vec : iov_base <%p>, iov_len %d", slot->vec.iov_base, slot->vec.iov_len);
    */
}

long bzdg_ioctl(struct file *filp, u_int cmd, u_long data)
{
    int i, err;
    struct sockaddr_in addr;
    struct sk_buff *skb = NULL;
    struct socket *sock = NULL;
    struct bzdg_info *info = NULL;

    info = filp->private_data;

    if(!info) {
        KMODD("There is no private in file structure.");
        return -EINVAL;
    }

    switch(cmd) {
        /* 
         * BZDG_CONNECT
         * 1. Copy IP address and Port number data from user space.
         *    Data must be passed by struct sockaddr_in format.
         *
         * 2. Call connect().
         *
         */
        case BZDG_CONNECT:
            err = copy_from_user(&addr, (struct sockaddr*)data, sizeof(struct sockaddr));
            if(err != 0) {
                KMODD("Copy of sockaddr data from user space failed.");
                return -EINVAL;
            }

            err = kernel_connect(info->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr), 0);

            if(err != 0) {
                KMODD("kernel_connect() failed.");
                return -EINVAL;
            }

            break;

        case BZDG_BIND:
            err = copy_from_user(&addr, (struct sockaddr*)data, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("Copy of sockaddr data from user space failed.");
                return -EINVAL;
            }

            err = kernel_bind(info->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr));

            if(err != 0) {
                KMODD("kernel_bind() failed.");
                return -EINVAL;
            }

            break; 

        case BZDG_SEND:
            for(i=0; i<BZDG_BATCH_NUM; i++) {
                if(!info->shmem[i].is_available) {
                    continue;
                }

                dump_slot(info->shmem[i]);
                skb = info->skbs[i];

                skb_reserve(skb, BZDG_HEAD_ROOM);
                skb_put(skb, info->shmem[i].vec.iov_len);

                sock = info->sock;
                sock->sk->sk_user_data = skb;

                skb_get(skb);
                err = kernel_sendmsg(sock, &(info->shmem[i].msg), &(info->shmem[i].vec), 1, info->shmem[i].vec.iov_len);
                KMODD("%d", err);

                bzdg_reset_skb(info, skb, i);
                info->shmem[i].is_available = 0;
            }

            break;

        case BZDG_GET_BATCH_NUM:
            return BZDG_BATCH_NUM;

        case BZDG_DBG:
            memset(info->shmem[0].bskb.data, 0, BZDG_BUFFER_SIZE);
            strcpy(info->shmem[0].bskb.data, "hogehoge");
            break;

        default:
            KMODD("Invalid argument.");
            return -EINVAL;
    }

    return 0;
}

static int bzdg_release(struct inode *inode, struct file *filp)
{
    struct bzdg_info *info = filp->private_data;
    KMODD("bzdg_release");
    sock_release(info->sock);
    kfree(filp->private_data);
    return 0;
}

static struct file_operations bzdg_fops = {
    .owner = THIS_MODULE,
    .open = bzdg_open,
    .mmap = bzdg_mmap,
    .unlocked_ioctl = bzdg_ioctl,
    .release = bzdg_release,
};

struct miscdevice bzdg_cdevsw = {
    MISC_DYNAMIC_MINOR,
    "bzdg",
    &bzdg_fops,
};

static int __init bzdg_module_init(void)
{
    misc_register(&bzdg_cdevsw);
    KMODD("bzdg loaded");
    return 0;
}

static void __exit bzdg_module_exit(void)
{
    misc_deregister(&bzdg_cdevsw);
    KMODD("bzdg unloaded");
    return;
}
/* -------------------------------------------------------------------- */

module_init(bzdg_module_init);
module_exit(bzdg_module_exit);

MODULE_AUTHOR("Yutaro Hayakawa");
MODULE_DESCRIPTION("BZDG");
MODULE_LICENSE("Dual BSD/GPL");
