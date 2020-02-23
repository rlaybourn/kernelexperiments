
obj-m += helloworld_rpi.o helloworld_rpi_char_driver.o helloworld_rpi_class_driver.o helloworld_rpi_misc_driver.o hellokeys_rpi.o hellokeys_rpi_ass_data.o

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


