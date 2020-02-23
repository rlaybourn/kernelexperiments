
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/of.h>

struct this_dev_data
{
	struct miscdevice hellokeydev;
	const char *name;
	int lastnum;
	char last_text[10];
};

ssize_t stringlen(char *instr)
{
	ssize_t counter = 0;
	pr_info("sringlen is called.\n");
	
	while(instr[counter] != '\0')
	{
		pr_info("%c %d\n",instr[counter],counter);
		counter++;
	}
	return counter;
}
static int my_dev_open(struct inode *inode, struct file *file)
{
	pr_info("my_dev_open() is called.\n");
	return 0;
}

static int my_dev_close(struct inode *inode, struct file *file)
{
	pr_info("my_dev_close() is called.\n");
	return 0;
}

static long my_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pr_info("my_dev_ioctl() is called. cmd = %d, arg = %ld\n", cmd, arg);
	return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos)
{
	struct this_dev_data *this_device;
	pr_info("write has been called\n");
	this_device = container_of(file->private_data,struct this_dev_data,hellokeydev);

	//if(copy_from_user(&in_text,buff,count))
	if(copy_from_user(this_device->last_text,buff,count))
	{
		pr_info("bad copy from userspace");
		return -EFAULT;
	}
	//in_text[count -1] = "\0";
	this_device->last_text[count -1] = '\0';
	pr_info("recieved data [%s] \n",this_device->last_text);
	return count;
}


static ssize_t dev_read(struct file *file, char __user *buff, size_t count, loff_t *ppos)
{
	struct this_dev_data *this_device;
	ssize_t textsise; 
	pr_info("device read ppos[%d] count[%d]:\n",(int)*ppos,(int)count);
	

	this_device = container_of(file->private_data,struct this_dev_data,hellokeydev);

	
	textsise = stringlen(this_device->last_text);
	/* If position is behind the end of a file we have nothing to read */
	if( *ppos >= textsise )
	{
			pr_info("behind end of file\n");
			return 0;
	}
	/* If a user tries to read more than we have, read only as many bytes as we have */
	if( *ppos + count > textsise )
	{
			count = textsise - *ppos;
			pr_info("more than we have [%d]\n",count);
	}
	if( copy_to_user(buff, this_device->last_text + *ppos, count) != 0 )
			return -EFAULT;
	/* Move reading position */
	*ppos += count;
	pr_info("leaving read [%d] [%d]\n",(int)*ppos,(int)count);
	return count;

}

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_dev_open,
	.release = my_dev_close,
	.read = dev_read,
	.write = dev_write,
	.unlocked_ioctl = my_dev_ioctl,
};

static struct miscdevice helloworld_miscdevice = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "mydev",
		.fops = &my_dev_fops,
};

/* Add probe() function */
static int __init my_probe(struct platform_device *pdev)
{
	struct this_dev_data *this_device;
	int ret_val;
	pr_info("my_probe() function is called.\n");

	this_device = devm_kzalloc(&pdev->dev, sizeof(struct this_dev_data), GFP_KERNEL);

	of_property_read_string(pdev->dev.of_node,"label",&this_device->name);

	this_device->hellokeydev.minor = MISC_DYNAMIC_MINOR;
	//this_device->hellokeydev.name = "mydev";
	this_device->hellokeydev.fops = &my_dev_fops;
	//ret_val = misc_register(&helloworld_miscdevice);
	this_device->hellokeydev.name = this_device->name;
	pr_info("name found is %s\n", this_device->name);
	ret_val = misc_register(&this_device->hellokeydev);

	if (ret_val != 0) {
		pr_err("could not register the misc device mydev");
		return ret_val;
	}
	platform_set_drvdata(pdev, this_device);
	pr_info("mydev: got minor %i\n",helloworld_miscdevice.minor);
	return 0;
}

/* Add remove() function */
static int __exit my_remove(struct platform_device *pdev)
{
	struct this_dev_data *this_device = platform_get_drvdata(pdev);
	pr_info("my_remove() function is called.\n");
	//misc_deregister(&helloworld_miscdevice);
	misc_deregister(&this_device->hellokeydev);
	return 0;
}

/* Declare a list of devices supported by the driver */
static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,hellokeys"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

/* Define platform driver structure */
static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "hellokeys",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

/* Register our platform driver */
module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("This is the simplest platform driver");
