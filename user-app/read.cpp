#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>

static void loading(void)
{
	static int i=0;
	const char s[4][3] = {"\b\\", "\b|", "\b/","\b-"};
	i++; // Allow overflow
	std::cerr 
		<< s[(i/128)%4] // '(i/128)%4' mean slow down the animation
		<< std::flush;
}



int net_init() 
{
	struct sockaddr_in server_addr = {}, client_addr = {};
	int portnum = 12345;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	char buf[100] = {};
	

	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_family 		= AF_INET;
	server_addr.sin_port 		= htons(portnum);
	if(sockfd < 0) {
		perror("Cannot open socket");
		exit(-1);
	}
	if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
		perror("bind() error");
		exit(1);
	}
	listen(sockfd, 1);
	unsigned int client_addr_len = sizeof(client_addr);
	int client_accepted = 
	accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
	if(client_accepted < 0) {
		perror("accept() error");
		exit(1);
	}
	int nread = read(client_accepted, buf, 100);
	if(nread < 0) {
		perror("read error");
		exit(1);
	}
	std::cout << "The message: " << buf << std::endl;
	const char *mess = "gotcha";
	int nwrite = write(client_accepted, mess, strlen(mess));
	if(write < 0) {
		perror("write error");
		exit(1);
	}
	shutdown(client_accepted, SHUT_RDWR);
	close(sockfd);
	return sockfd;
}

bool is_stdout_redirected() 
{
	if(!isatty(fileno(stdout))) return true;
	return false;
}

int main() 
{
	int32_t data;
	ssize_t ret;
	int fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
	std::cerr << "\nReadding... " << std::flush;
	net_init();
	if(fd<0) {
		perror("Module was not loaded");
		return -1;
	}
	bool redirected = is_stdout_redirected();
	while(ret = read(fd, &data, sizeof(int32_t) )) {
		if(ret == -1) {
			std::cerr << "\nThere was a fault when read from device\n"
				"Please restart\n";
			close(fd);
			return -1;
		}
		std::cout << data << std::endl;
		if(redirected) loading();
	}
	close(fd);
	std::cerr << std::endl;
	return 0;
}