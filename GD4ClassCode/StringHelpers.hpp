#pragma once
#include <sstream>

//Since std::to_string doesn't work in MinGW we implement our own
template<typename T>
std::string toString(const T& value);
#include "StringHelpers.inl"
