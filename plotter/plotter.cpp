#include <iostream>
#include <SFML/Network.hpp>


int main(int ac, char **av) {
	std::cout << "Waiting" << std::endl; 
	sf::TcpListener listener;
	if (listener.listen(12345) != sf::Socket::Done) {
		std::cerr << "listener: listen fail" << std::endl;
		return -1;
	}
	sf::TcpSocket client;
	if (listener.accept(client) != sf::Socket::Done) {
		std::cerr << "listener: fail to accept client" << std::endl;
		return -1;
	}
	char s[100]="abcdefsdfsdfsdsd";
	if (client.send(s, 100) != sf::Socket::Done) {
		std::cerr << "client socket: fail to send" << std::endl;
		return -1;
	}
	return 0;
}