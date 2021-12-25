#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

static void loading(void)
{
	static int i=0;
	const char s[4][3] = {"\b\\", "\b|", "\b/","\b-"};
	i++; // Allow overflow
	fprintf(stderr, "%s", s[(i/128)%4]); // '(i/128)%4' mean slow down the animation
	fflush(stderr);
}

int main() 
{
	int32_t data;
	ssize_t ret;
	int fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
	fprintf(stderr, "\nReading... ");
	fflush(stderr);

	if(fd<0) return fd;
	while(ret = read(fd, &data, sizeof(int32_t) )) {
		if(ret == -1) {
			fprintf(stderr, "\nThere was a fault when read from device\n"
				"Please restart\n");
			close(fd);
			return -1;
		}
		printf("%d\n", data);
		loading();
	}
	close(fd);
	fprintf(stderr, "\n");
	return 0;
}