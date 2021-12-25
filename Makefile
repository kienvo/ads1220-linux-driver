PWD := $(shell pwd)
obj-m += ads1220dev.o
ads1220dev-objs := driver.o ads1220.o devfile.o

KERNEL = kernel-header/linux-5.4.20/
CROSS = /usr/bin/arm-linux-gnueabihf-
# ccflags-y +=  -xc -E -v


#	# make -C kernel-header/linux-5.4.20 M=$(PWD) clean
#	# make -C kernel-header/linux-5.4.20 M=$(PWD) modules
all: mod apps

mod:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL) M=$(PWD) modules
	cp -rf ads1220dev.ko opz/spi/
clean:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL) M=$(PWD) clean
	make -C /user-app clean
	make -C /plotter clean

apps:
	make -C /user-app
	make -C /plotter