#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include<linux/slab.h> /* for using kzalloc() */
#include <linux/uaccess.h> /* for copy_from_user() */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kurt McAlpine <kurt@linux.com>");
MODULE_DESCRIPTION("In-kernel phrase reverser");

static unsigned long buffer_size = 8192;
module_param(buffer_size, ulong, (S_IRUSR | S_IRGRP | S_IROTH));
MODULE_PARM_DESC(buffer_size, "Internal buffer size");

struct buffer {
        wait_queue_head_t read_queue;
        char *data, *end, *read_ptr;
        unsigned long size;
};

static struct buffer *buffer_alloc(unsigned long size)
{
        struct buffer *buf = NULL;
        buf = kzalloc(sizeof(*buf), GFP_KERNEL);
        if (unlikely(!buf))
                goto out;

        init_waitqueue_head(&buf->read_queue);

        out:
                return buf;
}

static ssize_t reverse_read(struct file *file, char __user * out,
        size_t size, loff_t * off)
{
        struct buffer *buf = file->private_data;
        ssize_t result;
        while (buf->read_ptr == buf->end) {
                if (file->f_flags & O_NONBLOCK) {
                        result = -EAGAIN;
                        goto out;
                }
                if (wait_event_interruptible(buf->read_queue, buf->read_ptr != buf->end)) {
                        result = -ERESTARTSYS;
                        goto out;
                }
        }
        size = min(size, (size_t) (buf->end - buf->read_ptr));
        if (copy_to_user(out, buf->read_ptr, size)) {
                result = -EFAULT;
                goto out;
        }
        buf->read_ptr += size;
        result = size;
        out:
                return result;
}

static int reverse_open(struct inode *inode, struct file *file)
{
        int err = 0;
        file->private_data = buffer_alloc(buffer_size);

        return err;
}

static struct file_operations reverse_fops = {
        .owner = THIS_MODULE,
        .open = reverse_open,
        .llseek = noop_llseek
};

static struct miscdevice reverse_misc_device = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "reverse",
        .fops = &reverse_fops
};

static int __init reverse_init(void)
{
        if (!buffer_size)
                return -1;
        misc_register(&reverse_misc_device);
        printk(KERN_INFO
                "reverse device has been registered, buffer size is %lu bytes\n",
                buffer_size);
        return 0;
}

static void __exit reverse_exit(void)
{
        misc_deregister(&reverse_misc_device);
        printk(KERN_INFO "reverse device has been unregistered\n");
}

module_init(reverse_init);
module_exit(reverse_exit);
