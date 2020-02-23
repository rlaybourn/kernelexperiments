
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/types.h>

/* This structure will represent single device */
struct ioexp_dev {
	struct i2c_client * client;
	struct miscdevice ioexp_miscdevice;
	char checker[8];
	char name[8]; /* ioexpXX */
};



ssize_t showData(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client;
	struct ioexp_dev *private;
	char i2cbuf[28];
	char tempbuf[10] = {'1','e'};
	char start = 0x80;
	uint16_t clear;
	char *thename = attr->attr.name;
	int ret;

	dev_info(dev,"get client data\n");
	client = to_i2c_client(dev);
	dev_info(dev,"get private data\n");
	private = i2c_get_clientdata(client);

	//if(dev == NULL)
	//{
	//	return 0;
	//}
	dev_info(dev,"[%s]\n",dev->init_name);
	ret = i2c_master_send(client,&start,1);
	dev_info(dev,"read back\n");
	ret = i2c_master_recv(client,i2cbuf,28);
	clear  = (i2cbuf[20] << 8) + i2cbuf[21];

	dev_info(dev,"showData (%d)\n",clear);
	//char thereturn[5] = {'h','e','l','l','o'};
	int length = snprintf(tempbuf,9,"%d\n",clear);
	int len = 2;
	memcpy(buf,tempbuf,length);
	return length;
}

ssize_t getData(struct device *dev, struct device_attribute *attr, char *buf,size_t count)
{
	struct i2c_client *client;
	struct ioexp_dev * ioexp;
	char i2cbuf[28];
	char start = 0x80;

	char *thename = attr->attr.name;
	dev_info(dev,"GetData (%s)\n",thename);
	struct miscdevice *pdata = dev_get_drvdata(dev);

		

	ioexp = container_of(pdata,
			     struct ioexp_dev, 
			     ioexp_miscdevice);

	client = ioexp->client;
	dev_info(dev,"device is (%s)\n",pdata->name);
	dev_info(dev,"device is (%s)(%s)\n",ioexp->name,ioexp->checker);
	int ret = i2c_master_send(client,&start,1);
	dev_info(dev,"read back\n");
	ret = i2c_master_recv(client,i2cbuf,28);
	
	return count;
}

static DEVICE_ATTR(file1,(S_IWUSR | S_IRUGO),showData,getData);
static DEVICE_ATTR(file2,(S_IWUSR | S_IRUGO),showData,getData);

static struct attribute *dev_attrs[] = {
	&dev_attr_file1.attr,
	&dev_attr_file2.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name = "display_cs",
	.attrs = dev_attrs,
};

static const struct attribute_group *dev_attr_groups[] = {
	&dev_attr_group,
	NULL,
};

/* User is reading data from /dev/ioexpXX */
static ssize_t ioexp_read_file(struct file *file, char __user *userbuf,
                               size_t count, loff_t *ppos)
{
	int expval, size;
	char buf[28];
	char start = 0x80;
	int i;
	int ret;
	struct ioexp_dev * ioexp;

	ioexp = container_of(file->private_data,
			     struct ioexp_dev, 
			     ioexp_miscdevice);

	/* read IO expander input to expval */
	//expval = i2c_smbus_read_byte(ioexp->client);
	
	ret = i2c_master_send(ioexp->client,&start,1);
	expval = i2c_master_recv(ioexp->client,buf,28);
	if (expval < 0)
		return -EFAULT;

	for(i = 0; i < 28; i++)
	{
		
		dev_info(&ioexp->client->dev,"%x) reading %d \n",i,(int)buf[i]);
	}
	/* 
         * converts expval in 2 characters (2bytes) + null value (1byte)
	 * The values converted are char values (FF) that match with the hex
	 * int(s32) value of the expval variable.
	 * if we want to get the int value again, we have to
	 * do Kstrtoul(). We convert 1 byte int value to
	 * 2 bytes char values. For instance 255 (1 int byte) = FF (2 char bytes).
	 */
	//size = sprintf(buf, "%02x", expval);

	/* 
         * replace NULL by \n. It is not needed to have the char array
	 * ended with \0 character.
	 */
	//buf[size] = '\n';

	/* send size+1 to include the \n character */
	if(*ppos == 0){
		if(copy_to_user(userbuf, &buf[20], 8)){
			pr_info("Failed to return led_value to user space\n");
			return -EFAULT;
		}
		*ppos+=8;
		return 8;
	}

	return 0;
}

/* Writing from the terminal command line, \n is added */
static ssize_t ioexp_write_file(struct file *file, const char __user *userbuf,
                                   size_t count, loff_t *ppos)
{
	int ret,i;
	unsigned long val;
	char buf[10];
	struct ioexp_dev * ioexp;

	ioexp = container_of(file->private_data,
			     struct ioexp_dev, 
			     ioexp_miscdevice);

	dev_info(&ioexp->client->dev, 
		 "ioexp_write_file entered on %s\n", ioexp->name);

	dev_info(&ioexp->client->dev,
		 "we have written %zu characters\n", count); 

	if(copy_from_user(buf, userbuf, count)) {
		dev_err(&ioexp->client->dev, "Bad copied value\n");
		return -EFAULT;
	}

	for(i = 0; i < count;i++ )
	{
		dev_info(&ioexp->client->dev,"writing %d \n",(int)buf[i]);
	}

	dev_info(&ioexp->client->dev, "the value is %lu\n", val);
	
	dev_info(&ioexp->client->dev,"writing %d chars to i2c\n",count);
	ret = i2c_master_send(ioexp->client,buf,count);
	if (ret < 0)
		dev_err(&ioexp->client->dev, "the device is not found\n");

	dev_info(&ioexp->client->dev, 
		 "ioexp_write_file exited on %s\n", ioexp->name);

	return count;
}

static const struct file_operations ioexp_fops = {
	.owner = THIS_MODULE,
	.read = ioexp_read_file,
	.write = ioexp_write_file,
};


static int ioexp_probe(struct i2c_client * client,
		const struct i2c_device_id * id)
{
	static int counter = 0;

	struct ioexp_dev * ioexp;
	int ret;
	char buf[] = {0x80,0x03};

	/* Allocate new structure representing device */
	ioexp = devm_kzalloc(&client->dev, sizeof(struct ioexp_dev), GFP_KERNEL);

	/* Store pointer to the device-structure in bus device context */
	i2c_set_clientdata(client,ioexp);

	/* Store pointer to I2C device/client */
	ioexp->client = client;
	
	sysfs_create_group(&client->dev.kobj,&dev_attr_group);
	ret = i2c_master_send(ioexp->client,buf,2);
	/* Initialize the misc device, ioexp incremented after each probe call */
	sprintf(ioexp->name, "ioexp%02d", counter++); 
	sprintf(ioexp->checker, "valid"); 
	dev_info(&client->dev, 
		 "ioexp_probe is entered on %s\n", ioexp->name);

	ioexp->ioexp_miscdevice.name = ioexp->name;
	ioexp->ioexp_miscdevice.minor = MISC_DYNAMIC_MINOR;
	ioexp->ioexp_miscdevice.fops = &ioexp_fops;

	dev_info(&client->dev,"Ready to create device\n\n");
	/* Register misc device */
	ret = misc_register(&ioexp->ioexp_miscdevice);

	
	sysfs_create_group(&ioexp->ioexp_miscdevice.this_device->kobj,&dev_attr_group);
	
	dev_info(&client->dev, 
		 "ioexp_probe is exited on %s\n", ioexp->name);

	// platform_set_drvdata(pdev, ioexp);

	return ret;
}

static int ioexp_remove(struct i2c_client * client)
{
	struct ioexp_dev * ioexp;

	/* Get device structure from bus device context */	
	ioexp = i2c_get_clientdata(client);

	dev_info(&client->dev, 
		 "ioexp_remove is entered on %s\n", ioexp->name);

	/* Deregister misc device */
	sysfs_remove_group(&ioexp->ioexp_miscdevice.this_device->kobj,&dev_attr_group);
	misc_deregister(&ioexp->ioexp_miscdevice);

	
	sysfs_remove_group(&client->dev.kobj,&dev_attr_group);
	//sysfs_remove_group(&ioexp->ioexp_miscdevice.kobj,&dev_attr_group);

	dev_info(&client->dev, 
		 "ioexp_remove is exited on %s\n", ioexp->name);

	return 0;
}

static const struct of_device_id ioexp_dt_ids[] = {
	{ .compatible = "arrow,ioexp", },
	{ }
};
MODULE_DEVICE_TABLE(of, ioexp_dt_ids);

static const struct i2c_device_id i2c_ids[] = {
	{ .name = "ioexp", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, i2c_ids);

static struct i2c_driver ioexp_driver = {
	.driver = {
		.name = "ioexp",
		.owner = THIS_MODULE,
		.of_match_table = ioexp_dt_ids,
	},
	.probe = ioexp_probe,
	.remove = ioexp_remove,
	.id_table = i2c_ids,
};

module_i2c_driver(ioexp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("This is a driver that controls several i2c IO expanders");


