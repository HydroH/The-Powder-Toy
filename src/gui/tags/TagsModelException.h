#ifndef TAGSMODELEXCEPTION_H_
#define TAGSMODELEXCEPTION_H_

#include <string>
#include <exception>
#include "Format.h"

class TagsModelException {
	std::wstring message;
public:
	TagsModelException(std::string message_): message(format::StringToWString(message_)) {};
	TagsModelException(std::wstring message_): message(message_) {};
	const char * what() const throw() { return format::WStringToString(message).c_str(); };
	~TagsModelException() throw() {};
};

#endif /* TAGSMODELEXCEPTION_H_ */
