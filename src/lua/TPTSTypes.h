#ifndef TPTSTYPES_H_
#define TPTSTYPES_H_

#include <string>
#include <typeinfo>
#include "gui/interface/Point.h"
#include "Format.h"
#include "Lang.h"

enum ValueType { TypeNumber, TypeFloat, TypePoint, TypeString, TypeNull, TypeFunction };
typedef union { int num; float numf; std::string* str; ui::Point* pt; } ValueValue;

class GeneralException
{
protected:
	std::wstring exception;
public:
	GeneralException(std::string message){
		exception = format::StringToWString(message);
	}
	GeneralException(std::wstring message){
		exception = message;
	}
	std::string GetExceptionMessage() {
		return format::WStringToString(exception);
	}
	std::wstring GetWExceptionMessage() {
		return exception;
	}
};


class NumberType;
class FloatType;
class StringType;
class PointType;

class AnyType
{
protected:
	ValueType type;
	ValueValue value;
public:
	AnyType(ValueType type_, ValueValue value_);
	AnyType(const AnyType & v);
	operator NumberType();
	operator FloatType();
	operator StringType();
	operator PointType();
	ValueType GetType();
	std::string TypeName()
	{
		switch(type)
		{
		case TypeNumber:
			return "Number";
		case TypeFloat:
			return "Float";
		case TypePoint:
			return "Point";
		case TypeString:
			return "String";
		case TypeNull:
			return "Null";
		case TypeFunction:
			return "Function";
		default:
			return "Unknown";
		}
	}
	static std::string TypeName(ValueType type)
	{
		switch(type)
		{
		case TypeNumber:
			return "Number";
		case TypeFloat:
			return "Float";
		case TypePoint:
			return "Point";
		case TypeString:
			return "String";
		case TypeNull:
			return "Null";
		case TypeFunction:
			return "Function";
		default:
			return "Unknown";
		}
	}
	~AnyType();
};

class InvalidConversionException: public GeneralException
{
private:
	ValueType from;
	ValueType to;
public:
	InvalidConversionException(ValueType from_, ValueType to_):
	GeneralException(TEXT_EXCEPT_CONV_1 + format::StringToWString(AnyType::TypeName(from_)) + TEXT_EXCEPT_CONV_2 + format::StringToWString(AnyType::TypeName(to_)) + TEXT_EXCEPT_CONV_3), from(from_), to(to_) {
	}
};

class NumberType: public AnyType
{
public:
	NumberType(int number);
	int Value();
};

class FloatType: public AnyType
{
public:
	FloatType(float number);
	float Value();
};

class StringType: public AnyType
{
public:
	StringType(std::string string);
	std::string Value();
};

class PointType: public AnyType
{
public:
	PointType(ui::Point point);
	PointType(int pointX, int pointY);
	ui::Point Value();
};

#endif /* TPTSTYPES_H_ */
