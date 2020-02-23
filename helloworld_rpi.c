
#include <linux/module.h>
#include <linux/time.h>

static int num = 2;
static struct timeval start_time,end_time;

module_param(num,int,S_IRUGO);

static int __init hello_init(void)
{
	pr_info("Hello world init [%d]\n",num);
	do_gettimeofday(&start_time);
	pr_info("time in seconds [%ld]",start_time.tv_sec);
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("Hello world exit\n");
	do_gettimeofday(&end_time);
	pr_info("unloaded after [%ld] seconds\n",(end_time.tv_sec - start_time.tv_sec));
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("This is a print out Hello World module");


