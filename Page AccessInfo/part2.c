//<mtest.c>
#define MAX_PAGE 500
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>  
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/sched/task.h>
#include<linux/string.h>
#include<linux/init.h>
#include<linux/sched.h> 
#include<linux/ktime.h> 
#include<linux/uaccess.h>
#include<linux/seq_file.h>
unsigned long vir_addr[MAX_PAGE];
unsigned long vir_addr_start;
unsigned long vir_addr_end;
int accessed_times[MAX_PAGE];


static pte_t *get_pte(struct task_struct task,unsigned long addr)
{
    p4d_t *p4d;
    pud_t *pud;
	pmd_t *pmd;
	pgd_t *pgd;
	pte_t *pte;
	spinlock_t *ptl;
    unsigned long kernel_addr;
    struct mm_struct *mm = task.mm;
    down_read(&mm->mmap_sem);
	pgd=pgd_offset(mm,addr);
	if(pgd_none(*pgd)||unlikely(pgd_bad(*pgd))){
		goto out;
	}
    p4d = p4d_offset(pgd,addr);
    if(p4d_none(*p4d)||unlikely(p4d_bad(*p4d))){
		goto out;
	}
	pud=pud_offset(p4d,addr);
	if(pud_none(*pud)||unlikely(pud_bad(*pud))){
		goto out;
	}
	pmd = pmd_offset(pud,addr);
	if(pmd_none(*pmd)||unlikely(pmd_bad(*pmd))){
		goto out;
	}
	pte = pte_offset_map_lock(mm,pmd,addr,&ptl);
	if(!pte)
		goto out;
	if(!pte_present(*pte))
		goto unlock;
unlock:
	pte_unmap_unlock(pte,ptl);
out:
    up_read(&mm->mmap_sem);
    if (!pte)
        printk("Translation not Found.\n");
    else
        {
            return pte;
        }

}
/*Get the taskstruct from pid*/
static struct task_struct *heat_ts(int param)
{
    struct pid *heat_pid = find_get_pid(param);
	struct task_struct *ts = get_pid_task(heat_pid,PIDTYPE_PID);
	return ts;
	printk("sucess find ts.");
}
/*Count the heat and the total page*/
static void heat_count(int curr_pid)
{
    
	struct task_struct *curr_ts = heat_ts(curr_pid);
	struct mm_struct *curr_mm = curr_ts->mm;//Get the mm
	struct vm_area_struct *vma;
	struct page *page = NULL;
	unsigned long vaddr;
	unsigned long phys_addr;
	int filtered_page = 0;
	pte_t *tmp_pte;
	//VMA Filter
	printk("start");
	printk("Total page:%d",curr_mm->total_vm);
	printk("Stack page:%d",curr_mm->stack_vm);
	printk("Total VMA %d",curr_mm->map_count);
    for(vma = curr_mm->mmap; vma; vma=vma->vm_next)
        {
            int count = 0;
			if (vma->vm_start <= curr_mm->brk &&
			vma->vm_end >= curr_mm->start_brk) 
			{	vir_addr_start=vma->vm_start;
                vir_addr_end=vma->vm_end;
				printk("This is the HEAP Segment");
				printk("VMA start at %lx   end at %lx\n",vma->vm_start,vma->vm_end);
				for(vaddr=vma->vm_start; vaddr<vma->vm_end;vaddr+=PAGE_SIZE)
				{   printk("0x%lx",vaddr);
					vir_addr[count]=vaddr;
					int young_page = 0;
					tmp_pte = get_pte(*curr_ts, vaddr);
					if(!tmp_pte){continue;}
					if(pte_young(*tmp_pte)){
                    young_page++;
                    *tmp_pte = pte_mkold(*tmp_pte);
                    }
                    accessed_times[count]=young_page;
					count++;
				}
				printk("filtered page:%d",filtered_page);
			}
			else
            {
                continue;
            }
			
			}
		
			
}

static int proc_show(struct seq_file *m,void *v){
    int i=0;
    seq_printf(m, "Start/End vaddr:0x%08lx - 0x%08lx\n", vir_addr_start, vir_addr_end);
    for (i = 0; i<=50; ++i){
		if(vir_addr[i]!=0&& accessed_times[i]!=0)
        {seq_printf(m, "0x%lx page was accessed %d times\n", vir_addr[i], accessed_times[i]);}
    }

	return 0;
}
static int my_proc_open(struct inode *inode,struct file *file){
	return single_open(file,proc_show,NULL);
}
static ssize_t heat_proc_write(struct file *file,
const char __user * buffer,
size_t count, loff_t * data)
{
    int pid=0;
    char *tmp = kzalloc((count+1),GFP_KERNEL);
    if(!tmp)return -ENOMEM;
    if(copy_from_user(tmp,buffer,count)){
        kfree(tmp);
	    return EFAULT;
    }

    if(kstrtoint(tmp,10,&pid)!=0)
    {
        printk("invalid pid");
    }
	if(pid == 0){ //clear info
	printk("clear info\n");
	int i=0;
	for(i=0;i<MAX_PAGE;i++)
	{
		vir_addr[i]=0;
		accessed_times[MAX_PAGE]=0;
	}
	
    vir_addr_start=0;
	vir_addr_end=0;
	
	return count;
	}
    printk("pid is %d",pid);
    heat_count(pid);
    return count; 
}
static struct file_operations proc_heat_info_operations = {
.write= heat_proc_write,
.open = my_proc_open,
.read = seq_read,
.write = heat_proc_write
};
static struct proc_dir_entry *mtest_proc_entry;
static int __init mtest_init(void)
{
    mtest_proc_entry = proc_create("pageheat", 0777, NULL,&proc_heat_info_operations);  
    if (mtest_proc_entry == NULL) {  
        printk("Error creating proc entry/n");  
        return -1;  
    }  
    printk("create the filename pageheat init success\n");
    return 0;  
}
static void __exit mtest_exit(void)
{
    printk("exit the module......pageheat_exit\n");    
    remove_proc_entry("pageheat", NULL);  
}  
MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("mtest");  
MODULE_AUTHOR("Jiang Yiwei");  
module_init(mtest_init);
module_exit(mtest_exit);