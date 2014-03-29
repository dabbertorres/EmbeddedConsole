#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <string>
#include <utility>
#include <functional>
#include <map>
#include <deque>

#include <SFML/System/String.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>

namespace swift
{
	class Console : public sf::Drawable
	{
		public:
			Console(int w, int h, const sf::Font& f, const std::string& p = "[swift alpha]:");
			~Console();

			void update(char t);

			void addCommand(const std::string& c, std::function<const std::string(std::vector<std::string> args)> f);

			bool isActivated() const;
			void activate(bool a);

		private:
			// returns a string of output from the command
			const std::string handleCommand(const std::string& c);
			void draw(sf::RenderTarget& target, sf::RenderStates states) const;

			sf::RectangleShape background;

			const sf::Font& font;

			const sf::String promptStr;
			sf::Text prompt;

			sf::String commandStr;
			sf::Text command;
			
			sf::String outputStr;
			sf::Text output;

			//std::deque<sf::Text> lines;

			std::map<std::string, std::function<const std::string(std::vector<std::string> args)>> commandList;

			bool activated;
	};
}

#endif // CONSOLE_HPP
