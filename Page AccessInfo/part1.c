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

/*Pass the pid*/

static int param_pid;
module_param(param_pid,int,0660);

/* Translate the page get the pte */

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
}
/*Count the heat and the total page*/
static void heat_count(void)
{
	struct task_struct *curr_ts = heat_ts(param_pid);
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
			if (vma->vm_start <= curr_mm->brk &&
			vma->vm_end >= curr_mm->start_brk) 
			{	
				printk("This is the HEAP Segment");
				printk("VMA start at %lx   end at %lx\n",vma->vm_start,vma->vm_end);
				for(vaddr=vma->vm_start; vaddr<vma->vm_end;vaddr+=PAGE_SIZE)
				{
					filtered_page += 1; 
					int young_page = 0;
					s64 elapsed_ns=0;
					ktime_t time_start;
					time_start=ktime_get();
					tmp_pte = get_pte(*curr_ts, vaddr);
					elapsed_ns = ktime_to_ns(ktime_sub(ktime_get(), time_start)); 
					printk("Find page cost %lld ns\n",elapsed_ns);
					page = pfn_to_page(pte_pfn(*tmp_pte));
					get_page(page);
					phys_addr=(unsigned long)page_address(page);
	    			phys_addr+=(vaddr&~PAGE_MASK);
	    			//printk("The physical address is 0x%lx\n",phys_addr);
					if(pte_young(*tmp_pte)){
                    young_page++;
                    *tmp_pte = pte_mkold(*tmp_pte);
                    }
					if(young_page>0){
						printk("%lx\n",vaddr);
						printk("This page has been acessed %d times",young_page);}
					
				}
				printk("filtered page:%d",filtered_page);
			}
			else
            {
                continue;
            }
			
			}
		
			
}
			

	

   
static int __init mtest_init(void)
{
	printk("Load the module success\n");
	s64 elapsed_ns=0;
	ktime_t time_start;
	time_start=ktime_get();
	heat_count(); 
	elapsed_ns = ktime_to_ns(ktime_sub(ktime_get(), time_start)); 
	printk("The program cost %lld ns\n",elapsed_ns);
    return 0;  
}
static void __exit mtest_exit(void)
{
    printk("exit the module.....\n");    
}  
MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("page_heat");  
MODULE_AUTHOR("Jiang Yiwei");  
module_init(mtest_init);
module_exit(mtest_exit);




