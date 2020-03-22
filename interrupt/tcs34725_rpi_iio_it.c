
#include <linux/module.h>
#include <linux/platform_device.h> 	
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/interrupt.h> 		
#include <linux/gpio/consumer.h>


#define ioexp_DRV_NAME "ioexp"
static char *HELLO_KEYS_NAME = "PB_KEY";
#define PERSISTANCE_REG 0x0c


static const int adxl345_uscale = 38300;




struct ioexp_device {
	struct i2c_client *client;
	char name[8];
	int values[4];

};

static const struct iio_chan_spec ioexp_channel[] = {
{
	.type		= IIO_LIGHT,
	.indexed	= 1,
	.output		= 0,
	.channel	= 0,
	.modified   = 1,
	.channel2   = IIO_MOD_LIGHT_CLEAR,
	.scan_index = 0,
	.scan_type = {			
		.sign = 'u',				
		.realbits = 16,				
		.storagebits = 16,			
		.endianness = IIO_BE,		
	},	
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) ,
},{
	.type		= IIO_LIGHT,
	.indexed	= 1,
	.output		= 0,
	.channel	= 1,
	.modified   = 1,
	.channel2   = IIO_MOD_LIGHT_RED,
	.scan_index = 1,
		.scan_type = {			
		.sign = 'u',				
		.realbits = 16,				
		.storagebits = 16,			
		.endianness = IIO_BE,		
	},	
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) ,
},{
	.type		= IIO_LIGHT,
	.indexed	= 1,
	.output		= 0,
	.channel	= 2,
	.modified   = 1,
	.channel2   = IIO_MOD_LIGHT_GREEN,
	.scan_index = 2,
		.scan_type = {			
		.sign = 'u',				
		.realbits = 16,				
		.storagebits = 16,			
		.endianness = IIO_BE,		
	},	
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) ,
},{
	.type		= IIO_LIGHT,
	.indexed	= 1,
	.output		= 0,
	.channel	= 3,
	.modified   = 1,
	.channel2   = IIO_MOD_LIGHT_BLUE,
	.scan_index = 3,
		.scan_type = {			
		.sign = 'u',				
		.realbits = 16,				
		.storagebits = 16,			
		.endianness = IIO_BE,		
	},	
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) ,
}

};

static int channel_to_reg(int channel)
{
	int chan;
	chan = channel;
	chan = chan * 2;
	chan = chan + 0x14;
	return chan;
}

static u16 read_value_16(struct i2c_client *client,int addr)
{
	u8 outbuf[3];
	u8 inbuf[2];
	u16 regval;
	int ret;

	outbuf[0] = 0x80 + addr;

	ret = i2c_master_send(client, outbuf, 1);
	pr_info("sent addr %x \n",outbuf[0]);
	if (ret < 0)
		return ret;
	ret = i2c_master_recv(client,inbuf,2);
	pr_info("read values[%x][%x]\n",inbuf[0],inbuf[1]);
	regval = (inbuf[1] << 8) | inbuf[0];
	pr_info("constructed value %d\n",regval);
	return regval;
}

static int dumpRegs(struct i2c_client *client)
{
	u8 outbuf[3];
	u8 inbuf[27];
	u16 regval;
	int ret;
	int i;

	outbuf[0] = 0x80;
		ret = i2c_master_send(client, outbuf, 1);
	pr_info("sent addr %x \n",outbuf[0]);
	if (ret < 0)
		return ret;
	ret = i2c_master_recv(client,inbuf,27);
	for(i= 0; i< 27; i++)
	{
		pr_info("read values %d) [%x]\n",i,inbuf[i]);
	}
	
}

static int clearInterrupt(struct i2c_client *client)
{
	u8 outbuf[3];
	u8 inbuf[27];
	u16 regval;
	int ret;
	int i;

	outbuf[0] = 0xE6;
	ret = i2c_master_send(client, outbuf, 1);
	if(ret >= 0)
	{
		pr_info("sent clear interrupt %x \n",outbuf[0]);
	}

}

/* interrupt handler */
static irqreturn_t hello_keys_isr(int irq, void *data)
{
	int i,chan;
	u16 regval;
	struct ioexp_device *dev = data;
	dev_info(dev->client, "interrupt received. key: %s\n", HELLO_KEYS_NAME);
	for(i =0; i < 4; i++)
	{
		chan = channel_to_reg(i);;
		regval = read_value_16(dev->client,chan);
		dev->values[i] = sign_extend32(le16_to_cpu(regval), 31);
	}
	clearInterrupt(dev->client);
	return IRQ_HANDLED;
}

static int write_to_register(struct i2c_client *client,int addr,u8 value)
{
	int err;
	u8 outbuf[3];

	outbuf[0] = 0x80 + addr;
	outbuf[1] = value;

	err = i2c_master_send(client, outbuf, 2); /* write DAC value */
	if (err < 0) {
		dev_err(&client->dev, "failed to write to register %d",addr);
		return err;
	}
	return err;
}



static int ioexp_set_value(struct iio_dev *indio_dev, int val, int channel)
{
	struct ioexp_device *data = iio_priv(indio_dev);
	u8 outbuf[3];
	int ret;
	int chan;

	pr_info("entered set value\n");
	if (channel == 2)
		chan = 0x0F;
	else
		chan = channel;

	if (val >= (1 << 16) || val < 0)
		return -EINVAL;

	outbuf[0] = 0x80; /* write and update value */

	outbuf[1] = val & 0xff; /* LSB byte of dac_code */

	ret = i2c_master_send(data->client, outbuf, 2);
	
	if (ret < 0)
		return ret;
	else if (ret != 3)
		return -EIO;
	else
		return 0;
}

static int ioexp_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *channel,
			    int *val, int *val2, long mask)
{
	struct ioexp_device *data = iio_priv(indio_dev);
	u16 regval;
	int chan;

	pr_info("entered read raw\n");
	chan = channel_to_reg(channel->channel);
	
	switch (mask) {
	case IIO_CHAN_INFO_RAW:	
		pr_info("chan info raw\n");
		/*
		 * Data is stored in adjacent registers:
		 * ADXL345_REG_DATA(X0/Y0/Z0) contain the least significant byte
		 * and ADXL345_REG_DATA(X0/Y0/Z0) + 1 the most significant byte
		 * we are reading 2 bytes and storing in a __le16
		 */
		//regval = read_value_16(data->client,chan);
		regval = data->values[channel->channel];
		*val = sign_extend32(le16_to_cpu(regval), 31);
		pr_info("returned value %d\n",val);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		pr_info("read scale\n");
		*val = 0;
		*val2 = adxl345_uscale;
		clearInterrupt(data->client);
		return IIO_VAL_INT_PLUS_MICRO;

	default:
		return -EINVAL;
	}
}

static int ioexp_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *channel,
			       int val, int val2, long mask)
{
	int ret;
	int chan;


	chan = channel_to_reg(channel->channel);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		//ret = ioexp_set_value(indio_dev, val, chan->channel);
		pr_info("raw written [%d] to channel %d\n",val,chan);
		ret = 0;
		return ret;
	case IIO_CHAN_INFO_SCALE:
		pr_info("scale written [%d][%d] to channel %d\n",val,val2,chan);
		ret = 0;
		return ret;

	default:
		return -EINVAL;
	}
}

static const struct iio_info ioexp_info = {
	.read_raw = ioexp_read_raw,
	.write_raw = ioexp_write_raw,
	.driver_module = THIS_MODULE,
};


static irqreturn_t trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ioexp_device *data = iio_priv(indio_dev);

	s16 buf[8]; /* 16 bytes */
	int i, ret, j = 0, base = 0;
	s16 sample;
	u16 regval;
	int chan;

	pr_info("handling trigger [%ul] [%u]",indio_dev->active_scan_mask,indio_dev->masklength);
	/* read the channels that have been enabled from user space */
	for_each_set_bit(i, indio_dev->active_scan_mask, indio_dev->masklength) {
		pr_info("entered read raw\n");
		chan = channel_to_reg(i);
		pr_info("read from address [%x]\n",chan);

		/*
		 * Data is stored in adjacent registers:
		 * ADXL345_REG_DATA(X0/Y0/Z0) contain the least significant byte
		 * and ADXL345_REG_DATA(X0/Y0/Z0) + 1 the most significant byte
		 * we are reading 2 bytes and storing in a __le16
		 */
		regval = read_value_16(data->client,chan);

		sample = regval;
		pr_info("pushing %d to [%d]",sample,i);
		if (ret < 0)
			goto done;
		buf[j++] = sample;
	}
	pr_info("pushed-->");
	/* each buffer entry line is 6 bytes + 2 bytes pad + 8 bytes timestamp */
	iio_push_to_buffers_with_timestamp(indio_dev, buf, pf->timestamp);

done:
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static int ioexp_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	static int counter = 0;
	struct gpio_desc *gpio;
	struct iio_dev *indio_dev;
	struct ioexp_device *data;
	struct device *dev = &client->dev;
	int irq;

	u8 inbuf[3];
	u8 command_byte;
	int err;
	dev_info(&client->dev, "tcs_probe()\n");

	///////////////////////////////////////////////////////////////////////////////////
		/* First method to get the Linux IRQ number */
	gpio = devm_gpiod_get(dev, NULL, GPIOD_IN);
	if (IS_ERR(gpio)) {
		dev_err(dev, "gpio get failed\n");
		return PTR_ERR(gpio);
	}
	irq = gpiod_to_irq(gpio);
	if (irq < 0)
		return irq;
	dev_info(dev, "The IRQ number is: %d\n", irq);

		/* Second method to get the Linux IRQ number */
	// irq = platform_get_irq(client, 0);
	// if (irq < 0){
	// 	dev_err(dev, "irq is not available\n");
	// 	return -EINVAL;
	// }
	// dev_info(dev, "IRQ_using_platform_get_irq: %d\n", irq);



	
	//////////////////////////////////////////////////////////////////////////////////////
	command_byte = 0x80 | 0x00; /* Write and update register with value 0xFF*/
	inbuf[0] = command_byte;
	inbuf[1] = 0x03;
	inbuf[2] = 0xFF;

	/* allocate the iio_dev structure */
	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (indio_dev == NULL) 
		return -ENOMEM;
	
	data = iio_priv(indio_dev);	
	i2c_set_clientdata(client, data);
	data->client = client;
	//ret = i2c_master_send(data->client,inbuf,2);

	sprintf(data->name, "ioexp%02d", counter++);
	dev_info(&client->dev, "data_probe is entered on %s\n", data->name);

	indio_dev->name = data->name;
	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &ioexp_info;
	indio_dev->channels = ioexp_channel;
	indio_dev->num_channels = 4;
	indio_dev->modes = INDIO_DIRECT_MODE;

	err = write_to_register(client,0,0x13);
	//err = i2c_master_send(client, inbuf, 2); /* write DAC value */
	if (err < 0) {
		dev_err(&client->dev, "failed to init device");
		return err;
	}

	//zero threshhold registers
	err = write_to_register(client,4,0x00);
	err = write_to_register(client,5,0x00);
	err = write_to_register(client,6,0x00);
	err = write_to_register(client,7,0x00);

	//interrupt on every integration
	err = write_to_register(client,PERSISTANCE_REG,0x04);
	//err = i2c_master_send(client, inbuf, 2); /* write DAC value */
	if (err < 0) {
		dev_err(&client->dev, "failed to set persistance");
		return err;
	}	

	dumpRegs(client);

	err = devm_iio_triggered_buffer_setup(&client->dev, indio_dev, &iio_pollfunc_store_time,
					      trigger_handler, NULL);
	if (err) {
		dev_err(&client->dev, "unable to setup triggered buffer\n");
	}

	dev_info(&client->dev, "the dac answer is: %x.\n", err);
	
	err = devm_iio_device_register(&client->dev, indio_dev);
	if (err)
		return err;

	dev_info(&client->dev, "ioexp DAC registered\n");

		int ret_val = devm_request_threaded_irq(dev,irq,NULL,hello_keys_isr,
								IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
								HELLO_KEYS_NAME,data);
	if (ret_val) {
		dev_err(dev, "Failed to request interrupt %d, error %d\n", irq, ret_val);
		return ret_val;
	}

	//clear any current interrupt status
	clearInterrupt(client);
	return 0;
}

static int ioexp_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "ioexp_remove()\n");
	return 0;
}

static const struct of_device_id dac_dt_ids[] = {
	{ .compatible = "arrow,tcsiio", },
	{ }
};
MODULE_DEVICE_TABLE(of, dac_dt_ids);

static const struct i2c_device_id ioexp_id[] = {
	{ "tcsiio", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ioexp_id);

static struct i2c_driver ioexp_driver = {
	.driver = {
		.name	= ioexp_DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = dac_dt_ids,
	},
	.probe		= ioexp_probe,
	.remove		= ioexp_remove,
	.id_table	= ioexp_id,
};
module_i2c_driver(ioexp_driver);

MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("ioexp 16-bit DAC");
MODULE_LICENSE("GPL");
