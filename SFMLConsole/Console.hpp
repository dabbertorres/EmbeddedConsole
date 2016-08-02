#ifndef DBR_CNSL_CONSOLE_HPP
#define DBR_CNSL_CONSOLE_HPP

#include <vector>
#include <unordered_map>
#include <functional>

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

/*#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>*/

// sfml forward declarations
namespace sf
{
	class Drawable;
	class Transformable;
	class Font;
	class View;
	class Event;
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
			/*enum class Cursor
			{
				Block,
				Line,
				Underscore,
			};*/

			Console();

			Console(const sf::String& prompt, const sf::Font& font);

			/// \param size Size in characters
			Console(const sf::Vector2u& size, float charSize);
			Console(const sf::Vector2u& size, float charSize, const sf::String& prompt, const sf::Font& font);

			Console(const Console& other);
			Console(Console&& other);

			~Console() = default;

			/* actions */
			void update(const sf::Event& event);
			void clear();

			void addCommand(const sf::String& name, Command&& command);

			// true/false command does/doesn't exist
			bool run(const sf::String& entry);

			template<typename T>
			Console& operator<<(const T& t);

			/* properties functions */

			// the font (for best behavior) should be monospace!
			const sf::Font* getFont() const;
			void setFont(const sf::Font& font);

			/*sf::Vector2u cursorAt() const;
			void cursorAt(const sf::Vector2u& pos);*/

			float characterSize() const;
			void characterSize(float val);

			// provided if custom input handling is desired.
			// Could be used for providing user access to a scripting language
			// Called when an entry does not match a previously provided command or "clear"
			// all input is provided to the single argument
			EntryHandler entryHandler;

			/* data */
			bool hasFocus;

			sf::Color background;
			sf::Color baseForeground;

			/*sf::Time cursorBlinkPeriod;*/

		private:
			// convenience "do the work" called from other constructors
			// private to prevent a user from passing in an invalid (null) sf::Font
			Console(const sf::Vector2u& size, float charSize, const sf::String& prompt, const sf::Font* font);

			void addText(const sf::String& str, sf::Text::Style style, sf::Color foreground, bool newline = true);
			void newPrompt(bool top = false);
			void setupSizes();

			void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

			std::unordered_map<sf::String, Command> commands;

			const sf::Font* font;
			std::vector<sf::Text> texts;
			std::size_t entryBufferLen;		// number of sf::Texts that are part of the entry buffer (from the end of this->texts)

			// size a single character uses
			sf::Vector2f charSize;

			// size in characters of the console
			sf::Vector2u size;

			mutable sf::View contentView;

			/*sf::Vector2u cursorPosition;*/
			std::size_t lineNumber;
			/*sf::RectangleShape cursor;*/
			/*Cursor cursorType;*/

			sf::RectangleShape backgroundShape;

			sf::String prompt;

			// used in conjunction with clear() and newPrompt()
			bool noNewPrompt;

			/*bool drawCursor;
			sf::Clock blinkClock;*/
		};

		template<typename T>
		Console& Console::operator<<(const T& t)
		{
			std::ostringstream oss;
			oss << t;

			if(txt)
				texts.push_back(addText({0, lineNumber * charSize * 1.f / aspectRatio}, txt));

			return *this;
		}
	}
}

#endif
