#ifndef PREVIEWMODELEXCEPTION_H_
#define PREVIEWMODELEXCEPTION_H_

#include <string>
#include <exception>
#include "Format.h"
using namespace std;

struct PreviewModelException: public exception {
	wstring message;
public:
	PreviewModelException(string message_): message(format::StringToWString(message_)) {}
	PreviewModelException(wstring message_): message(message_) {}
	const char * what() const throw()
	{
		return format::WStringToString(message).c_str();
	}
	~PreviewModelException() throw() {};
};

#endif /* PREVIEWMODELEXCEPTION_H_ */
