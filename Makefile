obj-m += reverse.o

# KERNEL_SOURCE_PATH is used to secify the path of the kernel
# source you wish to build this module against, if it is not
# defined it will build against the kernel source in /lib
ifndef KERNEL_SOURCE_PATH
	KERNEL_SOURCE_PATH := /lib/modules/$(shell uname -r)/build
endif

all:
	make -C $(KERNEL_SOURCE_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE_PATH) M=$(PWD) clean
