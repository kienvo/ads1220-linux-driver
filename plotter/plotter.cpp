#include <iostream>
#include <SFML/Network.hpp>


int main(int ac, char **av) {
	const char *ip = "192.168.1.138";
	unsigned short port = 12345;
	std::cout << "Waiting" << std::endl; 
	sf::TcpSocket socket;
	sf::Socket::Status status = socket.connect(ip, port);
	if (status != sf::Socket::Done) {
		std::cerr << "Cannot connect to " << ip << ':' << port << std::endl;
		return 1;
	}
	// socket.send("sdssdsd", 7);
	// std::size_t recv;
	// char buf[100];
	// socket.receive(buf, 100, recv);
	// std::cerr << "Message is: " << buf << std::endl;

	while(1) {
		int32_t adcVal;
		std::size_t received;
		if(socket.receive(&adcVal, sizeof(int32_t), received)
			!= sf::Socket::Done) {
			std::cerr << "receive error " << std::endl;
			return 1;
		}
		std::cout << adcVal << std::endl;
	}
	return 0;
}