#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
static int __init hello_init(void)
{
    printk(KERN_INFO "Hello build\n");
    return 0;

}
static void __exit hello_exit(void)
{
    printk(KERN_INFO"Goodbye world\n");

}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiang Yiwei");
MODULE_DESCRIPTION("Hello test");
MODULE_ALIAS("Project1");