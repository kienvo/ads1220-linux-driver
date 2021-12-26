#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

#include <unistd.h>

#define FPS 0
#define BUFSZ 50
#define DISP_BUFSZ 2000
#define WINH 500
#define WINW 900
#define WAVE_OFFSETH	50
#define WAVE_OFFSETW	50
#define HEIGHT_SCALE ((WINH*1.0)/DISP_BUFSZ)
#define WIDTH_SCALE (((WINW-WAVE_OFFSETH*2)*1.0)/DISP_BUFSZ)
#define YSCALE_FACTOR ((WINH-WAVE_OFFSETH*2)*1.0)/0x07fffff

const char *ip = "192.168.1.138";
unsigned short port = 12345;

sf::RenderWindow *const window_init(int width, int height)
{
	sf::ContextSettings settings;
	auto * window = new sf::RenderWindow(sf::VideoMode(width, height), "Plotter", 
		sf::Style::Default, settings);
	window->setFramerateLimit(FPS);
	window->setPosition(sf::Vector2i(0,270));
	/* Revert the default coordinate */
	sf::View view = window->getDefaultView();
	view.setSize(width, -height);
	window->setView(view);
	return window;
}

void shift_wave(sf::VertexArray *wave) 
{
	for(int i=0; i<(DISP_BUFSZ - BUFSZ); i++) {
		sf::Vector2f newshift((*wave)[i].position.x,(*wave)[i+BUFSZ].position.y);
		(*wave)[i]=sf::Vertex(newshift, sf::Color::Red);
	}
}

void apply_buf(sf::VertexArray *wave, std::vector<int32_t>& buf)
{
	for(int i=(DISP_BUFSZ - BUFSZ); i<DISP_BUFSZ; i++) {

		sf::Vector2f newval((*wave)[i].position.x, YSCALE_FACTOR*buf[i-(DISP_BUFSZ - BUFSZ)]+WAVE_OFFSETH);
		(*wave)[i]= sf::Vertex(newval, sf::Color::Red);
	}
}
void EventHandler(sf::RenderWindow& window){
	sf::Event event;
	while(window.pollEvent(event)) {
		if(event.type == sf::Event::Closed) {
			window.close();
			exit(0);
		}
		else if(event.type == sf::Event::KeyPressed) {
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
				//
			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
				window.close();
				exit(0);
			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::W) && event.key.control){
				window.close();
				exit(0);
			}
		}
	}
}

int main(int ac, char **av) {

	auto * window = window_init(WINW, WINH);

	auto *wave = new sf::VertexArray(sf::LinesStrip, DISP_BUFSZ);
	for(int i=0; i<DISP_BUFSZ; i++) {
		(*wave)[i] = sf::Vertex(
			// sf::Vector2f(i*WIDTH_SCALE, i*HEIGHT_SCALE), sf::Color::Red
			sf::Vector2f(i*WIDTH_SCALE+WAVE_OFFSETW, 0+WAVE_OFFSETH), sf::Color::Red
		);
	}
	std::vector<int32_t> buf(BUFSZ, 0);
	window->draw(*wave);
	window->display();


	std::cerr << "Connecting to " << ip << ':' << port;
	sf::TcpSocket socket;
	sf::Socket::Status status = socket.connect(ip, port);
	if (status != sf::Socket::Done) {
		std::cerr << " Failed" << std::endl;
		return 1;
	}
	std::cerr << " Success" << std::endl;
	int ii=0;
	while(1) {
		int32_t adcVal;
		std::size_t received;
		if(socket.receive(&adcVal, sizeof(int32_t), received)
			!= sf::Socket::Done) {
			std::cerr << "receive error " << std::endl;
			return 1;
		}
		std::cout << adcVal << std::endl;
		buf[ii] = adcVal;
		ii++;
		if(ii>BUFSZ) {
			EventHandler(*window);
			ii=0;
			shift_wave(wave);
			apply_buf(wave, buf);
			window->clear();
			window->draw(*wave);
			window->display();
		}
	}
	return 0;
}