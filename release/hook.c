#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/kern_levels.h>
#include <linux/gfp.h>
#include <asm/unistd.h>
#include <asm/paravirt.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axel");
MODULE_DESCRIPTION("Simple Hooking Of a Syscall");
MODULE_VERSION("1.0");


unsigned long **SYS_CALL_TABLE;



void EnablePageWriting(void){
    write_cr0(read_cr0() & (~0x10000));

}
void DisablePageWriting(void){
    write_cr0(read_cr0() | 0x10000);

}


//define our origional function.
asmlinkage int ( *original_seek ) (unsigned int fd, unsigned long offset_high,
                   unsigned long offset_low, loff_t *result,
                   unsigned int whence);


//Create Our version of Open Function.
asmlinkage int HookSeek(unsigned int fd, unsigned long offset_high,
                   unsigned long offset_low, loff_t *result,
                   unsigned int whence){

    printk("File Seek:%d,%ld,%ld,%lld,%d", fd,offset_high,offset_low,*result,whence);

    return (*original_seek)(fd,offset_high,offset_low,result,whence);
}

// Set up hooks.
static int __init SetHooks(void) {
    // Gets Syscall Table **
    SYS_CALL_TABLE = (unsigned long**)kallsyms_lookup_name("sys_call_table");

    printk(KERN_INFO "Hooks Will Be Set.\n");
    printk(KERN_INFO "System call table at %p\n", SYS_CALL_TABLE);

  // Opens the memory pages to be written
    EnablePageWriting();

  // Replaces Pointer Of Syscall_open on our syscall.
    original_seek = (void*)SYS_CALL_TABLE[__NR_llseek];
    SYS_CALL_TABLE[__NR_llseek] = (unsigned long*)HookSeek;
    DisablePageWriting();

    return 0;
}

static void __exit HookCleanup(void) {

    // Clean up our Hooks
    EnablePageWriting();
    SYS_CALL_TABLE[__NR_llseek] = (unsigned long*)original_seek;
    DisablePageWriting();

    printk(KERN_INFO "HooksCleaned Up!");
}

module_init(SetHooks);
module_exit(HookCleanup);
