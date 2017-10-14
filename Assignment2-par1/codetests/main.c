#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>


int main(int argc, char **argv) {
	int fd;
	fd = open("/dev/HCSR_01",O_RDWR);
	if (fd < 0 ){
		printf("Can not open device file.\n");
		return -1;
	} else {
		write(fd,"",0);
		read(fd,"",0);
	}
	
	close(fd);

	return 0;
}
