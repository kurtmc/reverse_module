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

static inline char *reverse_word(char *start, char *end)
{
        char *orig_start = start, tmp;


        for (; start < end; start++, end--) {
                tmp = *start;
                *start = *end;
                *end = tmp;
        }
        
        return orig_start;
}

static char *reverse_phrase(char *start, char *end)
{
        char *word_start = start, *word_end = NULL;

        while ((word_end = memchr(word_start, ' ', end - word_start)) != NULL) {
                reverse_word(word_start, word_end - 1);
                word_start = word_end + 1;
        }

        reverse_word(word_start, end);

        return reverse_word(start, end);
}

static ssize_t reverse_write(struct file *file, const char __user * in,
        size_t size, loff_t * off)
{
        struct buffer *buf = file->private_data;
        ssize_t result;

        if (size > buffer_size) {
                result = -EFBIG;
                goto out;
        }

        if (copy_from_user(buf->data, in, size)) {
                result = -EFAULT;
                goto out;
        }

        buf->end = buf->data + size;
        buf->read_ptr = buf->data;

        if (buf->end > buf->data)
                reverse_phrase(buf->data, buf->end -1);
        
        wake_up_interruptible(&buf->read_queue);

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

static void buffer_free(struct buffer *buffer)
{
        kfree(buffer->data);
        kfree(buffer);
}

static int reverse_close(struct inode *inode, struct file *file)
{
        struct buffer *buf = file->private_data;

        buffer_free(buf);

        return 0;
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
