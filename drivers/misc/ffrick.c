#include <linux/module.h>
#include <linux/init.h>

static void __exit ffrick_exit (void)
{
	printk(KERN_INFO "ffrick_exit function done");
}

static int __init ffrick_init (void)
{
	printk(KERN_INFO "ffrick_init function done");
	return 0;
}

module_init(ffrick_init);
module_exit(ffrick_exit);

MODULE_AUTHOR("ffrick");
MODULE_DESCRIPTION("dummy test module");
MODULE_VERSION("0.1");
