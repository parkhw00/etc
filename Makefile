
obj-m += chardev.o

all: test_chardev
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

