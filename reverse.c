#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kurt McAlpine <kurt@linux.com>");
MODULE_DESCRIPTION("In-kernel phrase reverser");

static int __init reverse_init(void)
{
        printk(KERN_INFO "reverse device has been registered\n");
        return 0;
}

static void __exit reverse_exit(void)
{
        printk(KERN_INFO "reverse device has been unregistered\n");
}

module_init(reverse_init);
module_exit(reverse_exit);
