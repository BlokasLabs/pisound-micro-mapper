#include "utils.h"

std::string to_std_string(IControl::value_t value, IControl::Type t)
{
	std::string str;
	switch (t)
	{
	case IControl::Type::INT:
		str = std::to_string(value.i);
		break;
	case IControl::Type::FLOAT:
		str = std::to_string(value.f);
		break;
	default:
		str = "(unrecognized type)";
		break;
	}
	return str;
}
