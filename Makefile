ifneq ($(KERNELRELEASE),)
obj-m := fbmodule.o

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: module fbtest

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f fbtest
endif
