#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/uaccess.h>
#include<linux/seq_file.h>
#include<linux/slab.h>

static char *str = NULL;

static int my_proc_show(struct seq_file *m,void *v){
	seq_printf(m,"%s\n",str);
	return 0;
}

static ssize_t my_proc_write(struct file* file,const char __user *buffer,size_t count,loff_t *f_pos){
	char *tmp = kmalloc((count+1),GFP_KERNEL);
	if(!tmp)return -ENOMEM;
	if(copy_from_user(tmp,buffer,count)){
		kfree(tmp);
		return EFAULT;
	}
	str=tmp;
	return count;
}

static int my_proc_open(struct inode *inode,struct file *file){
	return single_open(file,my_proc_show,NULL);
}

static struct file_operations fops={
	.open = my_proc_open,
	.read = seq_read,
	.write = my_proc_write
};
static int __init hello_init(void){
	struct proc_dir_entry *parent = proc_mkdir("proc_test", NULL);//create directory named "proc_test"
	struct proc_dir_entry *entry;
	entry = proc_create("Hello_Proc",0666,parent,&fops);
	if(!entry){
		return -1;	
	}
	return 0;
}

static void __exit hello_exit(void){
	remove_proc_entry("Hello_Proc",NULL);
	printk(KERN_INFO "Goodbye world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");