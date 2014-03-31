#include "Console.hpp"

#include <sstream>

namespace swift
{
	Console::Console(int w, int h, const sf::Font& f, const std::string& p)
		:	background(sf::Vector2f(w, h)),
		    font(f),
		    promptStr(p),
		    activated(false)
	{
		background.setFillColor(sf::Color(0, 0, 0, 190));
		background.setOutlineThickness(5);
		background.setOutlineColor(sf::Color(60, 60, 60));

		prompt.setFont(font);
		prompt.setCharacterSize(14);
		prompt.setString(promptStr);

		command.setFont(font);
		command.setCharacterSize(14);
		command.setString(commandStr);
		command.setPosition(prompt.getGlobalBounds().width + 2, 0);
		
		output.setFont(font);
		output.setCharacterSize(14);
		output.setString(outputStr);
		output.setPosition(0, prompt.getGlobalBounds().height);
	}

	Console::~Console()
	{
	}

	void Console::update(char t)
	{
		if(activated)
		{
			// 8 = backspace, 13 = enter/return, 127 = delete
			if(!(t == 8 || t == 13 || t == 127))
			{
				commandStr += t;
				command.setString(commandStr);
				
				// wrap around
				if(command.getGlobalBounds().width + prompt.getGlobalBounds().width >= background.getGlobalBounds().width)
				{
					char temp = commandStr[commandStr.getSize() - 1];
					commandStr.erase(commandStr.getSize() - 1);
					commandStr += '\n';
					commandStr += temp;
					command.setString(commandStr);
				}
			}
			else if(t == 8)
			{
				if(commandStr.getSize() > 0)
					commandStr.erase(commandStr.getSize() - 1);
				command.setString(commandStr);
			}
			else if(t == 13)
			{
				if(commandStr == "")
					return;
				outputStr = handleCommand(commandStr);
				output.setString(outputStr);
				
				commandStr.clear();
				command.setString(commandStr);
			}
			else if(t == 127)
			{
				// for the time being, we don't want it to do anything
			}
		}
	}
			
	void Console::addCommand(const std::string& c, std::function<std::string(std::vector<std::string> args)> f)
	{
		commandList.emplace(c, f);
	}

	bool Console::isActivated() const
	{
		return activated;
	}

	void Console::activate(bool a)
	{
		activated = a;
	}
	
	Console& Console::operator <<(const std::string& str)
	{
		lines.emplace_front(str, font, 14);
		return *this;
	}

	std::string Console::handleCommand(const std::string& c)
	{
		std::istringstream ss(c);
		std::vector<std::string> args;
		
		int i = 0;
		std::string temp;
		while(ss >> temp)
		{
			args.push_back(temp);
			++i;
		}
		
		if(commandList.count(args[0]) != 0)
			return commandList[args[0]](std::move(args));
		else
			return "Unknown command";
	}

	void Console::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		if(activated)
		{
			target.draw(background);
			target.draw(prompt);
			target.draw(command);
			target.draw(output);
		}
	}
}
