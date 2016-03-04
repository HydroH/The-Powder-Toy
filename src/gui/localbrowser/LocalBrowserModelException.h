#ifndef STAMPSMODELEXCEPTION_H_
#define STAMPSMODELEXCEPTION_H_

#include <string>
#include <exception>
#include "Format.h"
using namespace std;

class LocalBrowserModelException {
	wstring message;
public:
	LocalBrowserModelException(string message_): message(format::StringToWString(message_)) {};
	LocalBrowserModelException(wstring message_): message(message_) {};
	const char * what() const throw() { return format::WStringToString(message).c_str(); };
	~LocalBrowserModelException() throw() {};
};

#endif /* STAMPSMODELEXCEPTION_H_ */
