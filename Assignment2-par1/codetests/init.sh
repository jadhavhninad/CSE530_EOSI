rmmod hcr_devices
insmod hcr_devices.ko
chmod 777 /dev/HCSR_01
chmod +x test.o
./test.o
