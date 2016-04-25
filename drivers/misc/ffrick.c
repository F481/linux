#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/io.h>

static u8* ffrick_mem;

static void __exit ffrick_exit (void)
{
	printk(KERN_INFO "%s\n", __func__);
	
	if (ffrick_mem)
		kfree(ffrick_mem);
}

static int __init ffrick_init (void)
{
	unsigned int i;
	
	ffrick_mem = kmalloc(64, GFP_KERNEL);

	printk(KERN_INFO "ffrick_mem: %p - %p\n",
		ffrick_mem, (void *) virt_to_phys(ffrick_mem));

	for (i = 0; i < 64; i++) {
		ffrick_mem[i] = 0;
	}
	ffrick_mem[0] = 1;
	printk(KERN_INFO "ffrick init function done");
	return 0;
}

module_init(ffrick_init);
module_exit(ffrick_exit);

MODULE_AUTHOR("ffrick");
MODULE_DESCRIPTION("dummy test module");
MODULE_VERSION("0.1");
