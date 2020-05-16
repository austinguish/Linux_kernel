#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
static int int_var;
static char* str_var;
static int int_array[8];
static int maxsize = 8;
module_param(str_var,charp,0660);
module_param(int_var,int,0660);
module_param_array(int_array,int,&maxsize,0660);
static int __init hello_init(void)
{
    int i=0;
    printk(KERN_INFO "Hello world\n");
    printk(KERN_INFO "string:%s;\n",str_var);
    printk(KERN_INFO "int:%d;\n",int_var);
    for(i=0;i<(sizeof int_array / sizeof (int));i++)
    {
        printk(KERN_INFO "int_array[%d]:%d",i,int_array[i]);
    }
    return 0;
}
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye world\n");
}
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("parameters");
MODULE_AUTHOR("Jiang Yiwei");
module_init(hello_init);
module_exit(hello_exit);
