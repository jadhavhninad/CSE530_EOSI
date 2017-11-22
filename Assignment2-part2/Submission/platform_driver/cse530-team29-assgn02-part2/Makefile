obj-m = Sample_platform_device.o Sample_platform_driver.o

all:
	make -Wall -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -Wall -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
