//<mtest.c>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/string.h>
#include<linux/vmalloc.h>
#include<linux/mm.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs.h>
#include<linux/seq_file.h>
#include<linux/slab.h>
/*Print all vma of the current process*/
static void mtest_list_vma(void)
{
    struct mm_struct *mm = current->mm;  
    struct vm_area_struct *vma;  
    down_read(&mm->mmap_sem); //Protect the memory 
    for (vma = mm->mmap;vma; vma = vma->vm_next) {  
        printk("start-0x%lx end-0x%lx %c%c%c \n",vma->vm_start, vma->vm_end,
         vma->vm_flags & VM_READ ? 'r' : '-',
         vma->vm_flags & VM_WRITE ? 'w' : '-',
         vma->vm_flags & VM_EXEC ? 'x' : '-');
    }// Traverse the VMA link_list.
    up_read(&mm->mmap_sem);   
}
/*Find va->pa translation */
static struct page *find_physical_page(struct vm_area_struct *vma,unsigned long addr){
	pud_t *pud;
    p4d_t *p4d;
	pmd_t *pmd;
	pgd_t *pgd;
	pte_t *pte;
	spinlock_t *ptl;
	struct page *page = NULL;
	struct mm_struct *mm = vma->vm_mm;
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
	page = pfn_to_page(pte_pfn(*pte));
	if(!page)
		goto unlock;
	get_page(page);
unlock:
	pte_unmap_unlock(pte,ptl);
out:
	return page;
}
static  mtest_find_page(unsigned long addr)
{
    p4d_t *p4d;
    pud_t *pud;
	pmd_t *pmd;
	pgd_t *pgd;
	pte_t *pte;
	spinlock_t *ptl;
    unsigned long kernel_addr;
    struct mm_struct *mm = current->mm;
    down_read(&mm->mmap_sem);
    struct vm_area_struct *vma;
    vma = find_vma(mm,addr);
	struct page *page = NULL;
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
	page = pfn_to_page(pte_pfn(*pte));
	if(!page)
		goto unlock;
	get_page(page);
unlock:
	pte_unmap_unlock(pte,ptl);
out:
    up_read(&mm->mmap_sem);
    if (!page)
        printk("Translation not Found.\n");
    else
        {kernel_addr=(unsigned long)page_address(page);
	    kernel_addr+=(addr&~PAGE_MASK);
	    printk("The physical address is 0x%lx\n",kernel_addr);
        }
}
static void   
mtest_write_val(unsigned long addr, unsigned long val)  
{  
    struct vm_area_struct *vma;  
    struct mm_struct *mm = current->mm;  
    struct page *page;  
    unsigned long kernel_addr;  
    down_read(&mm->mmap_sem);  
    vma = find_vma(mm, addr);  
    if (vma && addr >= vma->vm_start && (addr + sizeof(val)) < vma->vm_end) {  
        if (!(vma->vm_flags & VM_WRITE)) {  
            printk("This vma is not writable for 0x%lx\n", addr);  
            goto out;  
        }  
        page = find_physical_page(vma,addr);
        if (!page) {      
            printk("page not found  for 0x%lx\n", addr);  
            goto out;  
        }  
          
        kernel_addr = (unsigned long)page_address(page);  
        kernel_addr += (addr&~PAGE_MASK);   
        *(unsigned long *)kernel_addr = val;  
        put_page(page);
        printk("write 0x%lx to address 0x%lx\n", val, kernel_addr);} 
        else {  
        printk("no vma found for %lx\n", addr);  
    }  
out:  
    up_read(&mm->mmap_sem);  
}  
static ssize_t mtest_proc_write(struct file *file,
const char __user * buffer,
size_t count, loff_t * data)
{
    unsigned long val1, val2; 
    char *tmp = kzalloc((count+1),GFP_KERNEL);
    if(!tmp)return -ENOMEM;
    if(copy_from_user(tmp,buffer,count)){
        kfree(tmp);
	    return EFAULT;
    }
    if (memcmp(tmp, "listvma", 7) == 0)   
        mtest_list_vma(); 
    else if (memcmp(tmp, "findpage", 8) == 0){
        if (sscanf(tmp + 8, "%lx", &val1) == 1)
        mtest_find_page(val1); 
        }
    else if (memcmp(tmp, "writeval", 8) == 0){
        if (sscanf(tmp + 8, "%lx %lx", &val1, &val2) == 2)
        mtest_write_val(val1, val2);
        }
    else printk("No such command!Please enter again!");
    return count; 
}
static struct file_operations proc_mtest_operations = {
.write= mtest_proc_write
};
static struct proc_dir_entry *mtest_proc_entry;
static int __init mtest_init(void)
{
    mtest_proc_entry = proc_create("mtest", 0777, NULL,&proc_mtest_operations);  
    if (mtest_proc_entry == NULL) {  
        printk("Error creating proc entry/n");  
        return -1;  
    }  
    printk("create the filename mtest mtest_init sucess\n");
    return 0;  
}
static void __exit mtest_exit(void)
{
    printk("exit the module......mtest_exit\n");    
    remove_proc_entry("mtest", NULL);  
}  
MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("mtest");  
MODULE_AUTHOR("Jiang Yiwei");  
module_init(mtest_init);
module_exit(mtest_exit);