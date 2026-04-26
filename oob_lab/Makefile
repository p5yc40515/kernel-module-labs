obj-m += oob_lab.o

KDIR := /mnt/lab/kernels/practice

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean