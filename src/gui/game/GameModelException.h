#ifndef GAMEMODELEXCEPTION_H_
#define GAMEMODELEXCEPTION_H_

#include <string>
#include <exception>
#include "Format.h"
using namespace std;

struct GameModelException: public exception {
	wstring message;
public:
	GameModelException(string message_): message(format::StringToWString(message_)) {}
	GameModelException(wstring message_): message(message_) {}
	const char * what() const throw()
	{
		return format::WStringToString(message).c_str();
	}
	~GameModelException() throw() {};
};

#endif /* GAMEMODELEXCEPTION_H_ */
