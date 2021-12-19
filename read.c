#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

int main() 
{
	int32_t data;
	int fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
	if(fd<0) return fd;
	while(read(fd, &data, sizeof(int32_t) )) {
		printf("%d\n", data);

	}
	return 0;
}