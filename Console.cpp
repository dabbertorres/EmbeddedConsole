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
			// 8 = delete, 13 = enter/return
			if(!(t == 8 || t == 13) && t < 128)
			{
				commandStr += t;
				command.setString(commandStr);
			}
			else if(t == 8)
			{
				if(commandStr.getSize() > 0)
					commandStr.erase(commandStr.getSize() - 1);
				command.setString(commandStr);
			}
			else if(t == 13)
			{
				outputStr = handleCommand(commandStr);
				output.setString(outputStr);
				
				commandStr.clear();
				command.setString(commandStr);
			}
		}
	}
			
	void Console::addCommand(const std::string& c, std::function<const std::string(std::vector<std::string> args)> f)
	{
		commandList.insert(std::make_pair(c, f));
	}

	bool Console::isActivated() const
	{
		return activated;
	}

	void Console::activate(bool a)
	{
		activated = a;
	}

	const std::string Console::handleCommand(const std::string& c)
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
			return commandList[args[0]](args);
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
