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

static int BZDG_BATCH_NUM = 8;

module_param(BZDG_BATCH_NUM, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(BZDG_BATCH_NUM, "Number of packet batching.");

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
    char data[BZDG_BUFFER_SIZE];
};


/*
 * Name: bzdg_info
 * Description: This struct manages important objects for bzdg.
 */
struct bzdg_info {
    struct bzdg_slot *shmem;
    int shmem_size;
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
    err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);

    if(err) {
        KMODD("Socket create failed");
        return -EINVAL;
    }

    /* Keep socket object reference in file object.*/
    info->sock = sock;

    info->shmem = NULL;

    return 0;
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

    KMODD("size of shared memory : %ld", size);

    info->shmem = (struct bzdg_slot *)field;
    info->shmem_size = size;

    return 0;
}

/*
void dump_slot(struct bzdg_slot slot) {
    KMODD("slot <%p>", &slot);
    KMODD("msg : msg_name <%p>, msg_namelen %d, msg_iov <%p>, msg_iovlen %d", slot.msg.msg_name, slot.msg.msg_namelen, slot.msg.msg_iov, slot.msg.msg_iovlen);
    KMODD("vec : iov_base <%p>, iov_len %d", slot.vec.iov_base, slot.vec.iov_len);
}
*/

long bzdg_ioctl(struct file *filp, u_int cmd, u_long data)
{
    int i, err;
    struct sockaddr_in addr;
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

            if(err) {
                KMODD("Copy of sockaddr data from user space failed.");
                return err;
            }

            err = kernel_bind(info->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr));

            if(err) {
                KMODD("kernel_bind() failed.");
                return err;
            }

            break; 

        case BZDG_SEND:
            err = 0;

            for(i=0; i<BZDG_BATCH_NUM; i++) {
                if(!info->shmem[i].is_available) {
                    continue;
                }
                err = kernel_sendmsg(info->sock, &(info->shmem[i].msg), &(info->shmem[i].vec), 1, info->shmem[i].vec.iov_len);
                info->shmem[i].is_available = 0;
            }

            return err;

        case BZDG_GET_BATCH_NUM:
            return BZDG_BATCH_NUM;

        default:
            KMODD("Invalid argument.");
            return -EINVAL;
    }

    return 0;
}

static int bzdg_release(struct inode *inode, struct file *filp)
{
    struct bzdg_info *info = filp->private_data;

    if(info->sock) {
        KMODD("release socket at <%p>", info->sock);
        sock_release(info->sock);
    }

    //free_pages((unsigned long) info->shmem, info->shmem_size);

    kfree((struct bzdg_info *)filp->private_data);

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
    KMODD("bzdg loaded. batch:%d", BZDG_BATCH_NUM);
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
