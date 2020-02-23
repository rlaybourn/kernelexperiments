
obj-m += tcs34725_rpi_iio.o adxl345_rpi_iio.o

export EXTRA_CFLAGS := -std=gnu99

KERNEL_DIR ?= $(HOME)/linux

all:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) clean

deploy:
	scp *.ko root@10.0.0.10:


