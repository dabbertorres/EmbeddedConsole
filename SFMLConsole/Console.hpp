#ifndef DBR_CNSL_CONSOLE_HPP
#define DBR_CNSL_CONSOLE_HPP

#include <vector>
#include <array>
#include <unordered_map>
#include <functional>

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

// forward declarations
namespace sf
{
	class Event;
}

namespace dbr
{
	namespace sfml
	{
		class BitmapFont;
	}
}

// std::hash for SFML's String class
namespace std
{
	template<>
	struct hash<sf::String>
	{
		std::size_t operator()(const sf::String& s) const;
	};
}

namespace dbr
{
	namespace cnsl
	{
		class Console;

		using Args = std::vector<sf::String>;
		using Command = std::function<void(const Args& args)>;
		using EntryHandler = std::function<void(const sf::String&)>;

		class Console : public sf::Drawable, public sf::Transformable
		{
		public:
			Console(const sfml::BitmapFont& font);
			Console(const sf::String& prompt, const sfml::BitmapFont& font);

			/// \param size Size in characters
			/// \param charScale scaling factors of character size
			/// \param prompt Prompt string to use
			/// \param font a dbr::sfml::BitmapFont for text
			Console(sf::Vector2u size, sf::Vector2f charScale, const sf::String& prompt, const sfml::BitmapFont& font);

			Console(const Console& other);
			Console(Console&& other);

			~Console() = default;

			/* actions */

			void update(const sf::Event& event);
			void clear();

			void addCommand(const sf::String& name, Command&& command);

			// returns true/false command does/doesn't exist
			bool run(const sf::String& entry);

			template<typename T>
			Console& operator<<(const T& t);

			/* properties functions */

			const sfml::BitmapFont* getFont() const;
			void setFont(const sfml::BitmapFont& font);

			std::size_t cursorAt() const;
			void cursorAt(std::size_t idx);

			// provided if custom input handling is desired (ie: used for providing user access to a scripting language)
			// Called when an entry does not match a provided command or "clear"
			// all input is provided as a single argument
			EntryHandler entryHandler;

			/* data */

			bool hasFocus;

			sf::Color background;
			sf::Color baseForeground;

			sf::Time cursorBlinkPeriod;

		private:
			struct Cell
			{
				std::array<sf::Vertex, 4> vertices;

				void setColor(sf::Color color);
				void setPosition(sf::Vector2f pos);
				void setSize(sf::Vector2f size);
				void setTexCoord(sf::FloatRect rect);
			};

			void addChar(sf::Uint32 unicode);
			void addString(const sf::String& str);
			void clearBuffer();
			void deleteAt(std::size_t bufIdx);

			std::size_t bufferIndex();
			void bufferIndex(std::size_t idx);
			std::size_t nextLine();
			void newPrompt();

			void setupSizes();

			void addHistory(const sf::String& str);
			void useHistory();

			void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

			std::unordered_map<sf::String, Command> commands;

			std::vector<sf::String> entryHistory;
			std::size_t historyIndex;

			const sfml::BitmapFont* font;
			sf::String buffer;

			// amount to scale a single character by
			sf::Vector2f charScale;

			// size in characters of the console
			sf::Vector2u size;

			// text drawing container
			std::vector<Cell> cells;

			// content transformations
			mutable sf::View contentView;

			std::size_t cursorIndex;
			sf::RectangleShape cursor;

			sf::RectangleShape backgroundShape;

			sf::String prompt;

			mutable bool drawCursor;
			mutable sf::Clock blinkClock;
		};

		template<typename T>
		Console& Console::operator<<(const T& t)
		{
			std::ostringstream oss;
			oss << t;

			addString(oss.str());

			return *this;
		}
	}
}

#endif
