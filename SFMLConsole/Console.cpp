#include "Console.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Font.hpp>

#include <SFML/Window/Event.hpp>

#include "BitmapFont.hpp"

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
		Console::Console(const sfml::BitmapFont& font)
			: Console("$ ", font)
		{}

		Console::Console(const sf::String& prompt, const sfml::BitmapFont& font)
			: Console{{80, 24}, {1.f, 1.f}, prompt, font}
		{}

		Console::Console(sf::Vector2u size, sf::Vector2f charScale, const sf::String& prompt, const sfml::BitmapFont& font)
			: entryHandler{},
			hasFocus{false},
			background{0, 0, 0, 230},
			baseForeground{sf::Color::White},
			cursorBlinkPeriod{sf::milliseconds(500)},
			commands{},
			font{&font},
			size{size},
			cells{size.x * size.y},
			charScale{charScale},
			contentView{{0, 0, static_cast<float>(size.x * font.getGlyphSize().x * charScale.x), static_cast<float>(size.y * font.getGlyphSize().y * charScale.y)}},
			cursorIndex{0},
			cursor{{font.getGlyphSize().x * charScale.x, font.getGlyphSize().y * charScale.y}},
			backgroundShape{contentView.getSize()},
			prompt{prompt},
			drawCursor{false},
			blinkClock{}
		{
			backgroundShape.setFillColor(background);
			backgroundShape.setOutlineColor(baseForeground);
			backgroundShape.setOutlineThickness(1);

			auto charSize = font.getGlyphSize();
			auto realSize = sf::Vector2f{charScale.x * charSize.x, charScale.y * charSize.y};

			for(auto i = 0u; i < cells.size(); ++i)
			{
				sf::Vector2f topLeft = {static_cast<float>(i % size.x * realSize.x),
										static_cast<float>(i / size.x * realSize.y)};

				cells[i].setPosition(topLeft);
				cells[i].setSize(realSize);
				cells[i].setColor(background);	// background, because characters shouldn't be visible yet
			}

			cursor.setFillColor(baseForeground);

			addString(prompt);
		}

		Console::Console(const Console& other)
			: entryHandler{other.entryHandler},
			hasFocus{other.hasFocus},
			background{other.background},
			baseForeground{other.baseForeground},
			cursorBlinkPeriod{other.cursorBlinkPeriod},
			commands{other.commands},
			entryHistory(other.entryHistory),
			font{other.font},
			size{other.size},
			charScale{other.charScale},
			contentView{other.contentView},
			cursor{other.cursor},
			backgroundShape{other.backgroundShape},
			prompt{other.prompt},
			drawCursor{other.drawCursor},
			blinkClock{other.blinkClock}
		{}

		Console::Console(Console&& other)
			: entryHandler{std::move(other.entryHandler)},
			hasFocus{other.hasFocus},
			background{other.background},
			baseForeground{other.baseForeground},
			cursorBlinkPeriod{other.cursorBlinkPeriod},
			commands{std::move(other.commands)},
			entryHistory(std::move(other.entryHistory)),
			font{other.font},
			size{other.size},
			charScale{other.charScale},
			contentView{other.contentView},
			cursor{other.cursor},
			backgroundShape{other.backgroundShape},
			prompt{other.prompt},
			drawCursor{other.drawCursor},
			blinkClock{other.blinkClock}
		{
			other.font = nullptr;
		}

		void Console::update(const sf::Event& event)
		{
			switch(event.type)
			{
				case sf::Event::TextEntered:
				{
					// non-printable characters
					if(event.text.unicode < 0x20 || (0x7F <= event.text.unicode && event.text.unicode <= 0x100))
					{
						// check for control characters
						constexpr std::uint32_t BACKSPACE = '\b';
						constexpr std::uint32_t TAB = '\t';
						constexpr std::uint32_t NEW_LINE = '\n';
						constexpr std::uint32_t CARR_RETURN = '\r';

						switch(event.text.unicode)
						{
							case BACKSPACE:
							{
								// delete character previous to buffer index and move index back by 1
								auto buf = bufferIndex();
								if(buf > 0)
									deleteAt(buf - 1);

								break;
							}

							case TAB:
								// auto-complete

								break;

							case NEW_LINE:
							case CARR_RETURN:
							{
								cursorAt(nextLine());

								addHistory(buffer);

								// submit buffer as command
								if(!run(buffer))
									addString("Command does not exist\n");

								buffer.clear();
								newPrompt();

								break;
							}

							default:
								break;
						}
					}
					else
					{
						// printable character. add to current buffer

						buffer += event.text.unicode;
						addChar(event.text.unicode);
					}

					break;
				}

				case sf::Event::KeyReleased:
					switch(event.key.code)
					{
						case sf::Keyboard::Up:
							if(historyIndex > 0)
								--historyIndex;
							useHistory();
							break;

						case sf::Keyboard::Down:
							if(historyIndex < entryHistory.size() - 1)
								++historyIndex;
							useHistory();
							break;

						case sf::Keyboard::Left:
							bufferIndex(bufferIndex() - 1);
							break;

						case sf::Keyboard::Right:
							bufferIndex(bufferIndex() + 1);
							break;

						case sf::Keyboard::Home:
							bufferIndex(0);
							break;

						case sf::Keyboard::End:
							bufferIndex(buffer.getSize());
							break;

						case sf::Keyboard::Delete:
							deleteAt(bufferIndex());
							break;

						default:
							break;
					}
					break;

				default:
					break;
			}
		}

		void Console::clear()
		{
			buffer.clear();

			for(auto& c : cells)
			{
				c.setColor(background);
				c.setTexCoord({0, 0, 0, 0});
			}

			cursorAt(0);
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

		const sfml::BitmapFont* Console::getFont() const
		{
			return font;
		}

		void Console::setFont(const sfml::BitmapFont& font)
		{
			auto charSize = font.getGlyphSize();

			auto width = static_cast<float>(size.x * charScale.x * charSize.x);
			auto height = static_cast<float>(size.y * charScale.y * charSize.y);
			contentView.reset({0.f, 0.f, width, height});

			backgroundShape.setSize(contentView.getSize());

			this->font = &font;
		}

		std::size_t Console::cursorAt() const
		{
			return cursorIndex;
		}

		void Console::cursorAt(std::size_t idx)
		{
			cursorIndex = idx;

			auto charSize = font->getGlyphSize();
			cursor.setPosition(cursorIndex % size.x * charScale.x * charSize.x, cursorIndex / size.x * charScale.y * charSize.y);
		}

		void Console::addChar(sf::Uint32 unicode)
		{
			if(unicode == '\n')
			{
				cursorAt(nextLine());
			}
			else
			{
				// don't mess with spaces
				if(unicode != ' ')
				{
					// check for reaching end of screen
					if(cursorIndex >= size.x * size.y)
						clear();

					auto charSize = static_cast<sf::Vector2f>(font->getGlyphSize());
					auto& glyph = font->getTextureCoord(unicode);

					cells[cursorIndex].setTexCoord({static_cast<float>(glyph.x), static_cast<float>(glyph.y), charSize.x, charSize.y});
					cells[cursorIndex].setColor(baseForeground);
				}

				cursorAt(cursorIndex + 1);
			}
		}

		void Console::addString(const sf::String& str)
		{
			for(auto u : str)
				addChar(u);
		}

		void Console::clearBuffer()
		{
			bufferIndex(0);

			for(auto i = 0u; i < buffer.getSize(); ++i)
			{
				cells[cursorIndex + i].setTexCoord({0.f, 0.f, 0.f, 0.f});
				cells[cursorIndex + i].setColor(background);
			}
		}

		void Console::deleteAt(std::size_t bufIdx)
		{
			if(!buffer.isEmpty() && bufIdx < buffer.getSize())
			{
				clearBuffer();
				buffer.erase(bufIdx);
				addString(buffer);
				bufferIndex(bufIdx);
			}
		}

		std::size_t Console::bufferIndex()
		{
			return cursorIndex % size.x - prompt.getSize();
		}

		void Console::bufferIndex(std::size_t idx)
		{
			if(idx <= buffer.getSize())
				cursorAt(cursorIndex + idx - bufferIndex());
		}

		std::size_t Console::nextLine()
		{
			return cursorIndex + size.x - cursorIndex % size.x;
		}

		void Console::newPrompt()
		{
			addString(prompt);
			buffer.clear();
		}

		void Console::setupSizes()
		{

		}

		void Console::addHistory(const sf::String& str)
		{
			entryHistory.push_back(buffer);
			historyIndex = entryHistory.size();
		}

		void Console::useHistory()
		{
			clearBuffer();
			buffer = historyIndex < entryHistory.size() ? entryHistory[historyIndex] : "";
			addString(buffer);
		}

		void Console::draw(sf::RenderTarget& target, sf::RenderStates states) const
		{
			states.transform *= getTransform();
			target.draw(backgroundShape, states);

			auto prevView = target.getView();
			auto targetSize = target.getSize();

			auto thisPos = getPosition();
			auto thisScale = getScale();
			auto thisSize = backgroundShape.getSize();
			contentView.setViewport({thisScale.x * thisPos.x / targetSize.x, thisScale.y * thisPos.y / targetSize.y, thisScale.x * thisSize.x / targetSize.x, thisScale.y * thisSize.y / targetSize.y});
			contentView.setCenter(thisSize / 2.f + thisPos);

			target.setView(contentView);

			states.texture = &font->getTexture();

			// this cast is safe to do, since a Cell is just 4 sf::Vertex, and cells is contiguous memory
			target.draw(reinterpret_cast<const sf::Vertex*>(cells.data()), cells.size() * 4, sf::PrimitiveType::Quads, states);

			if(drawCursor)
				target.draw(cursor, states);

			target.setView(prevView);

			if(blinkClock.getElapsedTime() >= cursorBlinkPeriod)
			{
				drawCursor = !drawCursor;
				blinkClock.restart();
			}
		}

		void Console::Cell::setColor(sf::Color color)
		{
			for(auto& v : vertices)
				v.color = color;
		}

		void Console::Cell::setPosition(sf::Vector2f pos)
		{
			for(int i = vertices.size() - 1; i > 0; --i)
				vertices[i].position += (pos - vertices[0].position);
			vertices[0].position = pos;
		}

		void Console::Cell::setSize(sf::Vector2f size)
		{
			vertices[1].position = vertices[0].position + sf::Vector2f{size.x, 0};
			vertices[2].position = vertices[0].position + size;
			vertices[3].position = vertices[0].position + sf::Vector2f{0, size.y};
		}

		void Console::Cell::setTexCoord(sf::FloatRect rect)
		{
			vertices[0].texCoords = {rect.left, rect.top};
			vertices[1].texCoords = {rect.left + rect.width, rect.top};
			vertices[2].texCoords = {rect.left + rect.width, rect.top + rect.height};
			vertices[3].texCoords = {rect.left, rect.top + rect.height};
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
#	error This sf::String hash is only implemented for x86 or x64 architectures
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
