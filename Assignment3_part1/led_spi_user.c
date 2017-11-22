#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])


int main() 
{
	int i,j,fd;
	char wr_buf[]={0xff,0x00,0x1f,0x0f};
	

	//char wr_buf[]={0xff};
	//char rd_buf[10];

	/*if (argc<2) {
		printf("Usage:\n%s [device]\n", argv[0]);
		exit(1);
	}*/
					   	
	fd = open("/dev/spidev1.0", O_RDWR);
	if (fd<=0) { 
		printf("%s: Device %s not found\n");
		exit(1);
	}
	
	for(j=0; i<16; i++){
		if (write(fd, wr_buf, ARRAY_SIZE(wr_buf)) != ARRAY_SIZE(wr_buf))
			perror("Write Error");
		printf("%d\n",i);
		sleep(1);
	}

	 /* for(j=0; i<16; i++){
	 	 write(fd, "1", 1);
		 	perror("Write Error");
		 printf("%d\n",i);
		 sleep(1);
	}*/


	usleep(55);
	close(fd);
	return 0;
}
