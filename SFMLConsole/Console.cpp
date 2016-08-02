#include "Console.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Font.hpp>

#include <SFML/Window/Event.hpp>

static std::vector<sf::String> split(const sf::String& str, sf::Uint32 splitOn)
{
	std::vector<sf::String> ret;

	auto start = str.begin();

	auto sub = [&](auto last)
	{
		auto off = std::distance(str.begin(), start);
		auto len = std::distance(start, last);

		if(len != 0)
			ret.emplace_back(str.substring(off, len));
	};

	for(auto it = start; it != str.end(); ++it)
	{
		if(*it == splitOn)
		{
			sub(it);
			start = std::next(it);
		}
	}

	// last part of string if it does not end with a space
	if(start != str.end())
		sub(str.end());

	return ret;
}

namespace dbr
{
	namespace cnsl
	{
		Console::Console()
			: Console{{80, 24}, 20}
		{}

		Console::Console(const sf::String& prompt, const sf::Font& font)
			: Console{{80, 24}, 20, prompt, &font}
		{}

		Console::Console(const sf::Vector2u& size, float charSize)
			: Console{size, charSize, "$ ", nullptr}
		{}

		Console::Console(const sf::Vector2u& size, float charSize, const sf::String& prompt, const sf::Font& font)
			: Console{size, charSize, prompt, &font}
		{}

		Console::Console(const Console& other)
			: entryHandler{other.entryHandler},
			hasFocus{other.hasFocus},
			background{other.background},
			baseForeground{other.baseForeground},
//			cursorBlinkPeriod{other.cursorBlinkPeriod},
			commands{other.commands},
			font{other.font},
			texts{other.texts},
			entryBufferLen{other.entryBufferLen},
			size{other.size},
			charSize{other.charSize},
			contentView{other.contentView},
//			cursorPosition{other.cursorPosition},
			lineNumber{other.lineNumber},
//			cursor{other.cursor},
//			cursorType{other.cursorType},
			backgroundShape{other.backgroundShape},
			prompt{other.prompt},
			noNewPrompt{other.noNewPrompt}/*,
			drawCursor{other.drawCursor},
			blinkClock{other.blinkClock}*/
		{}

		Console::Console(Console&& other)
			: entryHandler{std::move(other.entryHandler)},
			hasFocus{other.hasFocus},
			background{other.background},
			baseForeground{other.baseForeground},
//			cursorBlinkPeriod{other.cursorBlinkPeriod},
			commands{std::move(other.commands)},
			font{other.font},
			texts{std::move(other.texts)},
			entryBufferLen{other.entryBufferLen},
			size{other.size},
			charSize{other.charSize},
			contentView{other.contentView},
//			cursorPosition{other.cursorPosition},
			lineNumber{other.lineNumber},
//			cursor{other.cursor},
//			cursorType{other.cursorType},
			backgroundShape{other.backgroundShape},
			prompt{other.prompt},
			noNewPrompt{other.noNewPrompt}/*,
			drawCursor{other.drawCursor},
			blinkClock{other.blinkClock}*/
		{
			other.entryHandler = nullptr;
			other.commands.clear();
			other.font = nullptr;
			other.texts.clear();
		}


		Console::Console(const sf::Vector2u& size, float charSize, const sf::String& prompt, const sf::Font* font)
			: entryHandler{},
			hasFocus{false},
			background{0, 0, 0, 230},
			baseForeground{sf::Color::White},
//			cursorBlinkPeriod{sf::milliseconds(250)},
			commands{},
			font{font},
			texts{},
			entryBufferLen{0},
			size{80, 24},
			charSize{0, charSize},
			contentView{{0, 0, static_cast<float>(size.x * charSize), static_cast<float>(size.y * charSize)}},
//			cursorPosition{0, 0},
			lineNumber{0},
//			cursor{},
//			cursorType{Cursor::Underscore},
			backgroundShape{contentView.getSize()},
			prompt{prompt},
			noNewPrompt{false}/*,
			drawCursor{false},
			blinkClock{}*/
		{
			backgroundShape.setFillColor(background);
			backgroundShape.setOutlineColor(baseForeground);
			backgroundShape.setOutlineThickness(1);

			if(font)
				setFont(*font);

			newPrompt(true);
		}

		void Console::update(const sf::Event& event)
		{
			switch(event.type)
			{
				case sf::Event::TextEntered:
				{
					auto& entry = texts.back();

					// non-printable unicode characters
					if(event.text.unicode < 0x20 || (0x7F <= event.text.unicode && event.text.unicode <= 0x100))
					{
						// check for control characters
						constexpr std::uint32_t BACKSPACE = '\b';
						constexpr std::uint32_t TAB = '\t';
						constexpr std::uint32_t NEW_LINE = '\n';
						constexpr std::uint32_t CARR_RETURN = '\r';
						constexpr std::uint32_t ESCAPE = 0x1B;

						switch(event.text.unicode)
						{
							case BACKSPACE:
							{
								// delete last character
								auto& string = entry.getString();
								entry.setString(string.substring(0, string.getSize() - 1));

								// if we emptied the last buffer, but we always want to keep at least 1 sf::Text for the entry
								if(string.isEmpty() && entryBufferLen > 1)
								{
									texts.pop_back();
									entry = texts.back();
									--entryBufferLen;
								}

								break;
							}

							case TAB:
								// auto-complete
								
								break;

							case NEW_LINE:
							case CARR_RETURN:
							{
								// submit buffer as command
								sf::String entry;

								auto start = texts.end() - 1;
								for(auto it = start; it != start - entryBufferLen; --it)
								{
									entry += it->getString();
								}

								if(!run(entry))
									addText("Command does not exist", sf::Text::Style::Regular, baseForeground);

								if(!noNewPrompt)
									newPrompt();

								noNewPrompt = false;

								break;
							}

							case ESCAPE:
								// cancel current buffer?
								
								break;

							default:
								break;
						}
					}
					else
					{
						// printable character. add to current buffer

						auto bounds = entry.getGlobalBounds();

						// if we extend past the width of the console...
						if(bounds.width - bounds.left + charSize.x >= backgroundShape.getGlobalBounds().width)
						{
							// add the following text on the next line
							addText(event.text.unicode, sf::Text::Style::Regular, baseForeground);
							++entryBufferLen;
						}
						else
						{
							entry.setString(entry.getString() + event.text.unicode);
						}
					}

					break;
				}

				case sf::Event::KeyReleased:
					switch(event.key.code)
					{
						case sf::Keyboard::Up:
							// back in history
							break;

						case sf::Keyboard::Down:
							// forward in history
							break;

						case sf::Keyboard::Left:
							// back in buffer
							break;

						case sf::Keyboard::Right:
							// forward in buffer
							break;

						default:
							break;
					}
					break;

				case sf::Event::MouseWheelScrolled:
					// scroll only with a vertical wheel
					if(event.mouseWheelScroll.wheel == sf::Mouse::Wheel::VerticalWheel)
					{
						// move the view
						event.mouseWheelScroll.delta;
					}
					break;

				default:
					break;
			}

			/*if(blinkClock.getElapsedTime() >= cursorBlinkPeriod)
			{
				drawCursor = !drawCursor;
				blinkClock.restart();
			}*/
		}

		void Console::clear()
		{
			texts.clear();
			newPrompt(true);
			noNewPrompt = true;
		}

		void Console::addCommand(const sf::String& name, Command&& command)
		{
			commands.emplace(name, command);
		}

		bool Console::run(const sf::String& entry)
		{
			auto args = split(entry, ' ');

			if(!args.empty())
			{
				auto it = commands.find(args.front());

				if(it != commands.end())
					it->second(args);
				else if(args.front() == "clear")
					clear();
				else if(entryHandler)
					entryHandler(entry);
				else
					return false;
			}

			return true;
		}

		const sf::Font* Console::getFont() const
		{
			return font;
		}

		void Console::setFont(const sf::Font& font)
		{
			auto glyph = font.getGlyph(prompt[0], static_cast<unsigned int>(charSize.y), false);
			charSize.x = glyph.advance;

			contentView.reset({0.f, 0.f, size.x * charSize.x, size.y * charSize.y});
			backgroundShape.setSize(contentView.getSize());

			this->font = &font;
		}

		/*sf::Vector2u Console::cursorAt() const
		{
			return cursorPosition;
		}

		void Console::cursorAt(const sf::Vector2u& pos)
		{
			cursorPosition = pos;
		}*/

		float Console::characterSize() const
		{
			return charSize.y;
		}

		void Console::characterSize(float val)
		{
			charSize.y = val;

			for(auto& t : texts)
				t.setCharacterSize(static_cast<unsigned int>(charSize.y));

			setupSizes();
		}

		void Console::addText(const sf::String& str, sf::Text::Style style, sf::Color foreground, bool newline)
		{
			// if we don't have a font, do nothing
			if(font == nullptr)
				return;

			sf::Text txt{str, *font, static_cast<unsigned int>(charSize.y)};

			sf::FloatRect prevBnds;
			if(!texts.empty())
			{
				prevBnds = texts.back().getGlobalBounds();
				prevBnds.top -= texts.back().getLocalBounds().top;
			}

			if(newline)
				txt.setPosition(0, prevBnds.top + std::max(prevBnds.height, charSize.y));
			else
				txt.setPosition(prevBnds.left + prevBnds.width, prevBnds.top);

			txt.setColor(foreground);
			txt.setStyle(style);

			texts.push_back(txt);
		}

		void Console::newPrompt(bool top)
		{
			addText(prompt, sf::Text::Style::Regular, baseForeground, !top);

			// new entry buffer
			addText("", sf::Text::Style::Regular, baseForeground, false);

			entryBufferLen = 1;
		}

		void Console::setupSizes()
		{

		}

		void Console::draw(sf::RenderTarget& target, sf::RenderStates states) const
		{
			states = states.transform * getTransform();
			target.draw(backgroundShape, states);

			auto prevView = target.getView();
			auto targetSize = target.getSize();

			auto thisPos = getPosition();
			auto thisScale = getScale();
			auto thisSize = backgroundShape.getSize();

			contentView.setViewport({thisPos.x / targetSize.x, thisPos.y / targetSize.y, thisScale.x * thisSize.x / targetSize.x, thisScale.y * thisSize.y / targetSize.y});

			target.setView(contentView);

			for(auto& t : texts)
				target.draw(t, states);

			/*if(drawCursor)
				target.draw(cursor, states);*/

			target.setView(prevView);
		}
	}
}

namespace std
{
	std::size_t hash<sf::String>::operator()(const sf::String& s) const
	{
		// FNV-1a hash (values for "prime" and "offset" from: www.isthe.com/chongo/tech/comp/fnv/#FNV-param)
		// (2 power of x) == 2 << (x - 1)

		// using the architecture detection used by nothings' stb libraries (www.github.com/nothings/stb)
#if defined(__x86_64__) || defined(_M_X64)
		// 64 bit
		constexpr std::size_t prime = (2u << 39) + (2u << 7) + 0xb3u;
		constexpr std::size_t offset = 14695981039346656037u;
#elif defined(__i386) || defined(_M_IX86)
		// 32 bit
		constexpr std::size_t prime = (2u << 23) + (2u << 7) + 0x93u;
		constexpr std::size_t offset = 2166136261u;
#else
#	error This sf::String hash is not implemented for non 32-bit or 64-bit architectures (Or, do you have weird compiler settings for some reason?)
#endif

		auto* ptr = reinterpret_cast<const std::uint8_t*>(s.getData());
		auto* end = ptr + s.getSize() * sizeof(sf::Uint32);

		std::size_t val = offset;

		for(; ptr != end; ++ptr)
		{
			val ^= *ptr;
			val *= prime;
		}

		return val;
	}
}
