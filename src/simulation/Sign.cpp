#include "Sign.h"
#include "graphics/Graphics.h"
#include "simulation/Simulation.h"
#include "Format.h"
#include "Lang.h"

sign::sign(std::string text_, int x_, int y_, Justification justification_):
	x(x_),
	y(y_),
	ju(justification_),
	text(format::StringToWString(text_))
{
}
sign::sign(std::wstring text_, int x_, int y_, Justification justification_):
	x(x_),
	y(y_),
	ju(justification_),
	text(text_)
{
}

std::string sign::getText(Simulation *sim)
{
	wchar_t buff[256];
	wchar_t signText[256];
	swprintf(signText, L"%s", text.substr(0, 255).c_str());

	if(signText[0] && signText[0] == '{')
	{
		if (!wcscmp(signText,L"{p}"))
		{
			float pressure = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				pressure = sim->pv[y/CELL][x/CELL];
			swprintf(buff, TEXT_GUI_SIGN_PRESS, pressure);  //...pressure
		}
		else if (!wcscmp(signText,L"{aheat}"))
		{
			float aheat = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				aheat = sim->hv[y/CELL][x/CELL];
			swprintf(buff, TEXT_GUI_SIGN_AHEAT, aheat-273.15);
		}
		else if (!wcscmp(signText,L"{t}"))
		{
			if (x>=0 && x<XRES && y>=0 && y<YRES && sim->pmap[y][x])
				swprintf(buff, TEXT_GUI_SIGN_TEMP, sim->parts[sim->pmap[y][x]>>8].temp-273.15);  //...temperature
			else
				swprintf(buff, TEXT_GUI_SIGN_TEMPNONE);  //...temperature
		}
		else
		{
			int pos = splitsign(signText);
			if (pos)
			{
				wcscpy(buff, signText+pos+1);
				buff[wcslen(signText)-pos-2]=0;
			}
			else
				wcscpy(buff, signText);
		}
	}
	else
	{
		wcscpy(buff, signText);
	}

	return format::WStringToString(buff);
}
std::wstring sign::getWText(Simulation *sim)
{
	wchar_t buff[256];
	wchar_t signText[256];
	swprintf(signText, L"%s", text.substr(0, 255).c_str());

	if(signText[0] && signText[0] == '{')
	{
		if (!wcscmp(signText,L"{p}"))
		{
			float pressure = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				pressure = sim->pv[y/CELL][x/CELL];
			swprintf(buff, TEXT_GUI_SIGN_PRESS, pressure);  //...pressure
		}
		else if (!wcscmp(signText,L"{aheat}"))
		{
			float aheat = 0.0f;
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				aheat = sim->hv[y/CELL][x/CELL];
			swprintf(buff, TEXT_GUI_SIGN_AHEAT, aheat);
		}
		else if (!wcscmp(signText,L"{t}"))
		{
			if (x>=0 && x<XRES && y>=0 && y<YRES && sim->pmap[y][x])
				swprintf(buff, TEXT_GUI_SIGN_TEMP, sim->parts[sim->pmap[y][x]>>8].temp-273.15);  //...temperature
			else
				swprintf(buff, TEXT_GUI_SIGN_TEMPNONE);  //...temperature
		}
		else
		{
			int pos = splitsign(signText);
			if (pos)
			{
				wcscpy(buff, signText+pos+1);
				buff[wcslen(signText)-pos-2]=0;
			}
			else
				wcscpy(buff, signText);
		}
	}
	else
	{
		wcscpy(buff, signText);
	}

	return std::wstring(buff);
}

void sign::pos(std::string signText, int & x0, int & y0, int & w, int & h)
{
	w = Graphics::textwidth(signText.c_str()) + 5;
	h = 15;
	x0 = (ju == Right) ? x - w :
		  (ju == Left) ? x : x - w/2;
	y0 = (y > 18) ? y - 18 : y + 4;
}
void sign::pos(std::wstring signText, int & x0, int & y0, int & w, int & h)
{
	w = Graphics::textwidth(signText.c_str()) + 5;
	h = 15;
	x0 = (ju == Right) ? x - w :
		(ju == Left) ? x : x - w/2;
	y0 = (y > 18) ? y - 18 : y + 4;
}

int sign::splitsign(const char* str, char * type)
{
	if (str[0]=='{' && (str[1]=='c' || str[1]=='t' || str[1]=='b' || str[1]=='s'))
	{
		const char* p = str+2;
		// signs with text arguments
		if (str[1] == 's')
		{
			if (str[2]==':')
			{
				p = str+4;
				while (*p && *p!='|')
					p++;
			}
			else
				return 0;
		}
		// signs with number arguments
		if (str[1] == 'c' || str[1] == 't')
		{
			if (str[2]==':' && str[3]>='0' && str[3]<='9')
			{
				p = str+4;
				while (*p>='0' && *p<='9')
					p++;
			}
			else
				return 0;
		}

		if (*p=='|')
		{
			int r = p-str;
			while (*p)
				p++;
			if (p[-1] == '}')
			{
				if (type)
					*type = str[1];
				return r;
			}
		}
	}
	return 0;
}
int sign::splitsign(const wchar_t* str, wchar_t * type)
{
	if (str[0]==L'{' && (str[1]==L'c' || str[1]==L't' || str[1]==L'b' || str[1]==L's'))
	{
		const wchar_t* p = str+2;
		// signs with text arguments
		if (str[1] == L's')
		{
			if (str[2]==L':')
			{
				p = str+4;
				while (*p && *p!=L'|')
					p++;
			}
			else
				return 0;
		}
		// signs with number arguments
		if (str[1] == L'c' || str[1] == L't')
		{
			if (str[2]==L':' && str[3]>=L'0' && str[3]<=L'9')
			{
				p = str+4;
				while (*p>=L'0' && *p<=L'9')
					p++;
			}
			else
				return 0;
		}

		if (*p==L'|')
		{
			int r = p-str;
			while (*p)
				p++;
			if (p[-1] == L'}')
			{
				if (type)
					*type = str[1];
				return r;
			}
		}
	}
	return 0;
}
