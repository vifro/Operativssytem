KDIR := /lib/modules/$(shell uname -r)/build

obj-m += netlinkKernelKO.o
netlinkKernelKO-objs := netlinkKernel.o parse_msg.o

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
