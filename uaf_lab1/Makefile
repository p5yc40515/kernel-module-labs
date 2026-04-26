obj-m += uaf_lab.o

KDIR 	:= /mnt/lab/kernels/practice
PWD 	:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean