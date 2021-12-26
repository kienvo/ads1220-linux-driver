#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

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

class net
{
private:
	int _sockfd, _port, accepted_client;
	struct sockaddr_in _server_addr = {}, _client_addr = {};
	unsigned int client_addr_len = sizeof(sockaddr_in);
public:
	net(int port);
	~net();
	int Listener() 
	{
		listen(_sockfd, 1);
		accepted_client = 
			accept(_sockfd, (struct sockaddr*)&_client_addr, &client_addr_len);
		if(accepted_client < 0) {
			perror("accept() error");
			exit(1);
		}
		return accepted_client;
	}
	int Read(char *buf, int count) 
	{
		int nread = read(accepted_client, buf, count);
		if(nread < 0) {
			perror("read error");
			exit(1);
		}
		return nread;
	}
	int Write(void const *buf, int count)
	{
		int nwrite = write(accepted_client, buf, count);
		if(nwrite < 0) {
			perror("write error");
			Shutdown();
		}
		return nwrite;
	}
	int WriteAsChar(int32_t num) 
	{
		auto s = std::to_string(num);
		s += '\n';	// TODO: temporarily let it here
		return Write(s.c_str(), s.length());
		
	}
	int Write(int32_t num) {
		return Write(&num, sizeof(int32_t));
	}
	inline void Shutdown() 
	{
		shutdown(accepted_client, SHUT_RDWR);
	}
};

net::net(int port):
_port(port)
{

	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(_sockfd < 0) {
		perror("Cannot open socket");
		exit(-1);
	}
	int option = 1;
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	_server_addr.sin_addr.s_addr	= INADDR_ANY;
	_server_addr.sin_family 		= AF_INET;
	_server_addr.sin_port 			= htons(port);
	if (bind(_sockfd, (struct sockaddr*)&_server_addr, sizeof(_server_addr))) {
		perror("bind() error");
		exit(1);
	}
}

net::~net()
{
	Shutdown();
	close(_sockfd);
}

void net_test() 
{
	net test(12345);
	test.Listener();
	char buf[200]={};
	test.Read(buf, 200);
	std::cerr << "Message is: " << buf << std::endl;

	const char *str = "Hello from Host\n";
	test.Write(str, strlen(str));
	std::cerr << "Message from Host has been sent" << std::endl;
}

bool is_stdout_redirected() 
{
	if(!isatty(fileno(stdout))) return true;
	return false;
}

void software_read() 
{
	int fd = open("/dev/ads1220", O_WRONLY|O_NONBLOCK);
	write(fd, "1", 1);
	close(fd);
}

int main() 
{
	int32_t data;
	ssize_t ret;
	// net_test();

	software_read();

	net *toPloter = new net(12345);
	toPloter->Listener();

	int fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
	std::cerr << "\nReadding... " << std::flush;

	if(fd<0) {
		perror("Module was not loaded");
		return -1;
	}
	bool redirected = is_stdout_redirected();
	while(ret = read(fd, &data, sizeof(int32_t) )) {
		if(ret == -1) {
			std::cerr << "\nThere was a fault when read from device\n"
				"Will be restart in a moment\n";
			close(fd);
			fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
			if(fd<0) {
				perror("Module was not loaded");
				return -1;
			}
		}
		std::cout << data << std::endl;
		int nwrite = toPloter->Write(data);
		if (nwrite < 0) {
			close(fd);	// For saving resource
			toPloter->Listener();
			fd = open("/dev/ads1220", O_RDONLY|O_NONBLOCK);
			if(fd<0) {
				perror("Module was not loaded");
				return -1;
			}
		}
		if(redirected) loading();
	}
	std::cerr << std::endl;
	close(fd);
	delete toPloter;
	return 0;
}