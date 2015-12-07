#ifndef __LINUX_KMOD_MODULE_H
#define __LINUX_KMOD_MODULE_H

#define KMOD_PROC_BUFLEN 4096

#define KMODD(fmt, ...) \
    printk(KERN_INFO "[%s]: "fmt"\n", __func__, ##__VA_ARGS__)

struct kmod_user {
    void *mem;
    unsigned long memsize;
    int cpu;
    struct list_head entity;
};

struct kmod_user_list {
    struct list_head head;
};

struct kmod_proc_fs {
    struct proc_dir_entry *entry;
    char buf[KMOD_PROC_BUFLEN];
    int buf_len;
    int buf_tmp;
    struct mutex mtx;
};

#endif
