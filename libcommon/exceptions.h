#pragma once

#include <string>

class Exception
{
public:
	explicit Exception(const std::string& what) : what_(what)
	{}

	const char* what() const
	{ return what_.c_str(); }

private:
	std::string what_;
};
