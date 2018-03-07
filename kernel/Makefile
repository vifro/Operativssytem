KDIR := /lib/modules/$(shell uname -r)/build

obj-m += nl_kw_modKO.o
nl_kw_modKO-objs := nl_kw_mod.o Parser/parse_msg.o TLV/tlv.o

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
