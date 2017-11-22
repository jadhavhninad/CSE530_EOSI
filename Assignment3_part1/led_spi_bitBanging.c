#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
	int fdexport, fd11;
	fdexport = open("/sys/class/gpio/export", O_WRONLY);
	if(fdexport < 0){
		printf("GPIO export failed");
	}

	write(fdexport, )



}
