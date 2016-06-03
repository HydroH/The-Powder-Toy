#ifndef SIGN_H_
#define SIGN_H_

#include <string>

class Simulation;

class sign
{
public:
	enum Justification { Left = 0, Middle = 1, Right = 2, None = 3 };
	sign(std::string text_, int x_, int y_, Justification justification_);
	sign(std::wstring text_, int x_, int y_, Justification justification_);
	int x, y;
	Justification ju;
	std::wstring text;

	std::string getText(Simulation *sim);
	std::wstring getWText(Simulation *sim);
	void pos(std::string signText, int & x0, int & y0, int & w, int & h);
	void pos(std::wstring signText, int & x0, int & y0, int & w, int & h);

	static int splitsign(const char* str, char * type = NULL);
	static int splitsign(const wchar_t* str, wchar_t * type = NULL);
};

#endif
