#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "Console.hpp"

int main(int argc, char** argv)
{
	using namespace dbr;

	sf::Font monoFont;
	monoFont.loadFromFile("consola.ttf");

	cnsl::Console console{"$ ", monoFont};
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