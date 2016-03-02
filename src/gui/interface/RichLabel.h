#pragma once

#include <string>

#include "Component.h"
#include "Colour.h"

namespace ui
{
	class RichLabel : public Component
	{
	public:
		struct RichTextRegion
		{
			int start;
			int finish;
			int action;
			std::wstring actionData;
		};

		RichLabel(Point position, Point size, std::string richText);
		RichLabel(Point position, Point size, std::wstring richText);

		virtual ~RichLabel();

		virtual void SetText(std::string text);
		virtual void SetText(std::wstring text);
		virtual std::string GetDisplayText();
		virtual std::wstring GetWDisplayText();
		virtual std::string GetText();
		virtual std::wstring GetWText();

		virtual void Draw(const Point& screenPos);
		virtual void OnMouseClick(int x, int y, unsigned button);
	protected:
		std::wstring textSource;
		std::wstring displayText;

		std::vector<RichTextRegion> regions;

		void updateRichText();
	};
}
