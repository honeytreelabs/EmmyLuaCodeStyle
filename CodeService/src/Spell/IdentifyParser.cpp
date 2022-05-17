#include "CodeService/Spell/IdentifyParser.h"

const int EOZ = -1;

IdentifyParser::IdentifyParser(std::string_view source)
	: _source(source),
	  _currentIndex(0)
{
}

void IdentifyParser::Parse()
{
	std::size_t start = 0;
	while (true)
	{
		switch (Lex())
		{
		case IdentifyType::Unknown:
		case IdentifyType::Unicode:
			{
				_words.clear();
				return;
			}
		case IdentifyType::Ascii:
			{
				WordRange range(start, _currentIndex - start);
				PushWords(range);
				start = _currentIndex;
				break;
			}
		case IdentifyType::LowerEnd:
			{
				WordRange range(start, _source.size() - start);
				PushWords(range);
				return;
			}
		case IdentifyType::Ignore:
			{
				++_currentIndex;
				start = _currentIndex;
				break;
			}
		default: // end
			{
				return;
			}
		}
	}
}

std::vector<IdentifyParser::Word>& IdentifyParser::GetWords()
{
	return _words;
}

IdentifyParser::IdentifyType IdentifyParser::Lex()
{
	enum class ParseState
	{
		Unknown,
		Number,
		LowerCase,
		UpperCase
	} state = ParseState::Unknown;

	std::size_t start = _currentIndex;

	while (true)
	{
		int ch = GetCurrentChar();

		if (ch == EOZ)
		{
			if(state == ParseState::LowerCase)
			{
				return IdentifyType::LowerEnd;
			}
			return IdentifyType::End;
		}

		if (ch == '_')
		{
			return IdentifyType::Ignore;
		}

		if (ch > std::numeric_limits<char>::max())
		{
			return IdentifyType::Unicode;
		}

		if (!std::isalnum(ch))
		{
			return IdentifyType::Unknown;
		}

		switch (state)
		{
		case ParseState::Unknown:
			{
				if (std::isdigit(ch))
				{
					state = ParseState::Number;
				}
				else if (std::isupper(ch))
				{
					state = ParseState::UpperCase;
				}
				else if (std::islower(ch))
				{
					state = ParseState::LowerCase;
				}

				break;
			}
		case ParseState::Number:
			{
				if (std::isdigit(ch))
				{
					break;
				}
				return IdentifyType::Ignore;
			}
		case ParseState::LowerCase:
			{
				if (std::islower(ch))
				{
					break;
				}
				return IdentifyType::Ascii;
			}
		case ParseState::UpperCase:
			{
				if (std::isupper(ch))
				{
					break;
				}
				else if (std::islower(ch))
				{
					if (_currentIndex - start == 1)
					{
						state = ParseState::LowerCase;
						break;
					}
					--_currentIndex;
				}

				return IdentifyType::Ignore;
			}
		}
		++_currentIndex;
	}
}


int IdentifyParser::GetCurrentChar()
{
	if (_currentIndex < _source.size())
	{
		unsigned char ch = _source[_currentIndex];
		return ch;
	}
	return EOZ;
}

void IdentifyParser::PushWords(WordRange range)
{
	std::string_view wordView = _source.substr(range.Start, range.Count);
	std::string word;
	word.resize(wordView.size());
	for (std::size_t i = 0; i != wordView.size(); i++)
	{
		word[i] = std::tolower(wordView[i]);
	}

	_words.emplace_back(range, word);
}
