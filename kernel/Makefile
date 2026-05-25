obj-m += position.o

KDIR := /home/leoff/linux-6.15.5
ARCH := riscv
CROSS_COMPILE := riscv64-linux-gnu-

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean