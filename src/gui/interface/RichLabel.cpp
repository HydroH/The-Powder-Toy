#include <vector>
#include <exception>

#include "RichLabel.h"
#include "Platform.h"
#include "gui/interface/Point.h"
#include "gui/interface/Component.h"
#include "graphics/Graphics.h"
#include "Format.h"
#include "Lang.h"

using namespace ui;

struct RichTextParseException: public std::exception {
	std::wstring message;
public:
	RichTextParseException(std::string message_): message(format::StringToWString(message_)) {}
	RichTextParseException(std::wstring message_ = TEXT_EXCEPT_RICHTEXT_DEFAULT): message(message_) {}
	const char * what() const throw()
	{
		return format::WStringToString(message).c_str();
	}
	~RichTextParseException() throw() {};
};

RichLabel::RichLabel(Point position, Point size, std::string labelText):
	Component(position, size),
	textSource(format::StringToWString(labelText)),
	displayText(L"")
{
	updateRichText();
}

RichLabel::RichLabel(Point position, Point size, std::wstring labelText):
	Component(position, size),
	textSource(labelText),
	displayText(L"")
{
	updateRichText();
}

RichLabel::~RichLabel()
{

}

void RichLabel::updateRichText()
{
	regions.clear();
	displayText = L"";

	if(textSource.length())
	{

		enum State { ReadText, ReadData, ReadRegion, ReadDataStart };
		State state = ReadText;

		int currentDataPos = 0;
		wchar_t * currentData = new wchar_t[textSource.length()+1];
		std::fill(currentData, currentData+textSource.length()+1, 0);

		int finalTextPos = 0;
		wchar_t * finalText = new wchar_t[textSource.length()+1];
		std::fill(finalText, finalText+textSource.length()+1, 0);

		int originalTextPos = 0;
		wchar_t * originalText = new wchar_t[textSource.length()+1];
		std::copy(textSource.begin(), textSource.end(), originalText);
		originalText[textSource.length()] = 0;

		int stackPos = -1;
		RichTextRegion * regionsStack = new RichTextRegion[256];

		try
		{
			while(originalText[originalTextPos])
			{
				wchar_t current = originalText[originalTextPos];

				if(state == ReadText)
				{
					if(current == L'{')
					{
						if(stackPos > 255)
							throw RichTextParseException(TEXT_EXCEPT_RICHTEXT_NEST);
						stackPos++;
						regionsStack[stackPos].start = finalTextPos;
						regionsStack[stackPos].finish = finalTextPos;
						state = ReadRegion;
					}
					else if(current == L'}')
					{
						if(stackPos >= 0)
						{
							currentData[currentDataPos] = 0;
							regionsStack[stackPos].actionData = std::wstring(currentData);
							regions.push_back(regionsStack[stackPos]);
							stackPos--;
						}
						else
						{
							throw RichTextParseException(TEXT_EXCEPT_RICHTEXT_BRACE);
						}
					}
					else
					{
						finalText[finalTextPos++] = current;
						finalText[finalTextPos] = 0;
						if(stackPos >= 0)
						{
							regionsStack[stackPos].finish = finalTextPos;
						}
					}
				}
				else if(state == ReadData)
				{
					if(current == L'|')
					{
						state = ReadText;
					}
					else
					{
						currentData[currentDataPos++] = current;
						currentData[currentDataPos] = 0;
					}
				}
				else if(state == ReadDataStart)
				{
					if(current != L':')
					{
						throw RichTextParseException(TEXT_EXCEPT_RICHTEXT_COLON);
					}
					state = ReadData;
					currentDataPos = 0;
				}
				else if(state == ReadRegion)
				{
					if(stackPos >= 0)
					{
						regionsStack[stackPos].action = current;
						state = ReadDataStart;
					}
					else
					{
						throw RichTextParseException();
					}
				}

				originalTextPos++;
			}

			if(stackPos != -1)
				throw RichTextParseException(TEXT_EXCEPT_RICHTEXT_UNCLOSED);

			finalText[finalTextPos] = 0;
			displayText = std::wstring(finalText);
		}
		catch (const RichTextParseException & e)
		{
			displayText = L"\br[Parse exception: " + format::StringToWString(e.what()) + L"]";
			regions.clear();
		}
		delete[] currentData;
		delete[] finalText;
		delete[] originalText;
		delete[] regionsStack;
	}
	TextPosition(displayText);
}

void RichLabel::SetText(std::string text)
{
	textSource = format::StringToWString(text);
	updateRichText();
}

void RichLabel::SetText(std::wstring text)
{
	textSource = text;
	updateRichText();
}

std::string RichLabel::GetDisplayText()
{
	return format::WStringToString(displayText);
}

std::wstring RichLabel::GetWDisplayText()
{
	return displayText;
}

std::string RichLabel::GetText()
{
	return format::WStringToString(textSource);
}

std::wstring RichLabel::GetWText()
{
	return textSource;
}

void RichLabel::Draw(const Point& screenPos)
{
	Graphics * g = GetGraphics();
	ui::Colour textColour = Appearance.TextInactive;
	g->drawtext(screenPos.X+textPosition.X, screenPos.Y+textPosition.Y, displayText, textColour.Red, textColour.Green, textColour.Blue, 255);
}

void RichLabel::OnMouseClick(int x, int y, unsigned button)
{
	int cursorPosition = Graphics::CharIndexAtPosition((wchar_t*)displayText.c_str(), x-textPosition.X, y-textPosition.Y);
	for(std::vector<RichTextRegion>::iterator iter = regions.begin(), end = regions.end(); iter != end; ++iter)
	{
		if((*iter).start <= cursorPosition && (*iter).finish >= cursorPosition)
		{
			switch((*iter).action)
			{
				case 'a':
					Platform::OpenURI(format::WStringToString((*iter).actionData));
				break;
			}
		}
	}
}
