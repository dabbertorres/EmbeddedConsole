#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "Console.hpp"
#include "BitmapFont.hpp"

int main(int argc, char** argv)
{
	using namespace dbr;

	sfml::BitmapFont monoFont;
	monoFont.loadFromFile("res/font12.png", {12, 12});

	cnsl::Console console{monoFont};
	console.setPosition(30, 30);

	sf::RenderWindow window{{1280, 720}, "SFML Console"};

	while(window.isOpen())
	{
		sf::Event event;
		while(window.pollEvent(event))
		{
			switch(event.type)
			{
				case sf::Event::Closed:
					window.close();
					break;

				case sf::Event::KeyPressed:
					if(event.key.code == sf::Keyboard::Escape)
						window.close();
					break;

				default:
					break;
			}

			console.update(event);
		}

		window.clear();
		window.draw(console);
		window.display();
	}

	return 0;
}